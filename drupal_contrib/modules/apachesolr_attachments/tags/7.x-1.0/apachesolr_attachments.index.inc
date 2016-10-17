<?php

/**
 * @file
 * Indexing-related functions.
 */

/**
 * Callback for apachesolr_index_nodes().
 *
 * Adds a document for each indexable file attachment for the given node ID.
 *
 * @see apachesolr_node_to_document()
 */
function apachesolr_attachments_add_documents($node, $namespace) {
  $documents = array();
  $hash = apachesolr_site_hash();
  // Let any module exclude this node from the index.
  $build_document = TRUE;
  foreach (module_implements('apachesolr_node_exclude') as $module) {
    $exclude = module_invoke($module, 'apachesolr_node_exclude', $node, $namespace);
    if (!empty($exclude)) {
      $build_document = FALSE;
    }
  }

  if ($build_document) {
    // Since there is no notification for an attachment being unassociated with a
    // node (but that action will trigger it to be indexed again), we check for
    // fids that were added before but no longer present on this node.
    $fids = array();
    $result = db_query("SELECT fid FROM {apachesolr_attachments_files} WHERE nid = :nid", array(':nid' => $node->nid));
    foreach ($result as $row) {
      $fids[$row->fid] = $row->fid;
    }

    $files = apachesolr_attachments_get_indexable_files($node);

    // Find deleted files.
    $missing_fids = array_diff_key($fids, $files);
    if ($missing_fids) {
      db_update('apachesolr_attachments_files')->fields(array('removed' => 1))->condition('fid', $missing_fids, 'IN')->execute();
    }
    $new_files = array_diff_key($files, $fids);

    // Add new files.
    foreach ($new_files as $file) {
      db_insert('apachesolr_attachments_files')->fields(array(
        'fid' => $file['fid'],
        'nid' => $node->nid,
        'removed' => 0,
        'hash' => '',
      ))->execute();
    }

    foreach ($files as $file) {
      $text = apachesolr_attachments_get_attachment_text($file);
      if ($text) {
        $document = new ApacheSolrDocument();
        // A single file might be attached to multiple nodes.
        $document->id = apachesolr_document_id($file['fid'] . '-' . $node->nid, 'file');
        $document->site = url(NULL, array('absolute' => TRUE));
        $document->hash = $hash;
        $document->entity_type = 'file';
        $document->entity_id = $file['fid'];
        $document->bundle = $node->type;
        $document->bundle_name = node_type_get_name($node);
        $document->label = $file['filename'];
        $document->is_nid = $node->nid;
        $document->url = file_create_url($file['uri']);
        $document->path = file_stream_wrapper_get_instance_by_uri($file['uri'])->getDirectoryPath() . '/' . file_uri_target($file['uri']);

        $document->content = $file['filename'] . ' ' . apachesolr_clean_text($file['description']) . ' ' . $text;

        $document->ss_name = $node->name;
        // We want the name to ale be searchable for keywords.
        $document->tos_name = $node->name;

        // Everything else uses dynamic fields
        $document->is_uid = $node->uid;
        $document->bs_status = $node->status;
        $document->bs_sticky = $node->sticky;
        $document->bs_promote = $node->promote;
        $document->is_tnid = $node->tnid;
        $document->bs_translate = $node->translate;
        if (empty($node->language)) {
          // 'und' is the language-neutral code in Drupal 7.
          $document->ss_language = LANGUAGE_NONE;
        }
        else {
          $document->ss_language = $node->language;
        }
        $document->ds_created = apachesolr_date_iso($file['timestamp']);
        $document->ds_changed = $document->ds_created;

        // apachesolr_attachments-specific fields.
        $document->ss_filemime = $file['filemime'];
        $document->ss_file_node_title = apachesolr_clean_text($node->title);
        $document->ss_file_node_url = url('node/' . $node->nid, array('absolute' => TRUE));

        // Add taxonomy to document.
        $indexed_fields = apachesolr_entity_fields('node');
        foreach ($indexed_fields as $index_key => $field_info) {
          if ($field_info['field']['type'] == 'taxonomy_term_reference') { // Add only taxonomy.
            $field_name = $field_info['field']['field_name'];
            // See if the node has fields that can be indexed
            if (isset($node->{$field_name})) {
              // Got a field.
              $function = $field_info['indexing_callback'];
              if ($function && function_exists($function)) {
                // NOTE: This function should always return an array.  One
                // node field may be indexed to multiple Solr fields.
                $fields = $function($node, $field_name, $index_key, $field_info);
                foreach ($fields as $field) {
                  // It's fine to use this method also for single value fields.
                  $document->setMultiValue($field['key'], $field['value']);
                }
              }
            }
          }
        }

        // Let modules add to the document.
        foreach (module_implements('apachesolr_update_index') as $module) {
          $function = $module . '_apachesolr_update_index';
          $function($document, $node, $namespace);
        }
        drupal_alter('apachesolr_attachments_index', $document, $node, $file, $namespace);

        $documents[] = $document;
      }
      else {
        watchdog('Apache Solr Attachments', 'Could not extract any indexable text from %filepath', array('%filepath' => $file['filepath']), WATCHDOG_WARNING);
      }
    }
  }
  return $documents;
}

/**
 * Return all non-excluded file attachments for a particular node
 */
function apachesolr_attachments_get_indexable_files($node) {
  $files = array();

  $field_names = apachesolr_attachments_get_field_names();
  foreach ($field_names as $field_name) {
    if (!empty($node->$field_name)) {
      $field = $node->$field_name;
      list($lang, $values) = each($field);
      $files = array_merge($files, $values);
    }
  }

  $file_list = array();
  foreach ($files as $file) {
    if (apachesolr_attachments_allowed_mime($file['filemime'])) {
      $file_list[$file['fid']] = $file;
    }
  }
  return $file_list;
}

/**
 * Checks if a file is of a MIME type that is to be excluded from the index.
 *
 * The MIME types of excluded files are built and cached each time the file
 * attachments settings form is saved.
 *
 * @param $filename
 *   A string, the MIME type of the file.
 *
 * @return
 *   Boolean, TRUE if the $filemime is allowed, otherwise FALSE.
 */
function apachesolr_attachments_allowed_mime($filemime) {

  $excluded = variable_get('apachesolr_attachments_excluded_mime', FALSE);
  if ($excluded === FALSE) {
    // Build the list of excluded MIME types.
    $excluded = array();
    $extensions = variable_get('apachesolr_attachments_excluded_extensions', FALSE);
    if ($extensions !== FALSE) {
      $extensions = explode(' ', $extensions);
    }
    else {
      $extensions = apachesolr_attachments_default_excluded();
    }
    foreach ($extensions as $ext) {
      $ext = trim($ext);
      if ($ext) {
        $mime = file_get_mimetype('dummy.' . $ext);
        $excluded[$mime] = 1;
      }
    }
    variable_set('apachesolr_attachments_excluded_mime', $excluded);
  }
  return empty($excluded[$filemime]);
}

/**
 * Return all fields that are of type 'file'
 */
function apachesolr_attachments_get_field_names() {
  $field_names = array();
  if (module_exists('file')) {
    $fields = field_info_fields();
    foreach ($fields as $field_name => $field) {
      if ($field['type'] == 'file') {
        $field_names[] = $field_name;
      }
    }
  }
  return $field_names;
}

/**
 * Parse the attachment getting just the raw text.
 *
 * @throws Exception
 */
function apachesolr_attachments_get_attachment_text($file) {
  $filepath = drupal_realpath($file['uri']);
  // Check that we have a valid filepath.
  if (!($filepath) || !is_file($filepath)) {
    return FALSE;
  }

  // No need to use java for plain text files.
  if ($file['filemime'] == 'text/plain' || $file['filemime'] == 'text/x-diff') {
    $text = file_get_contents($filepath);
    // TODO - try to detect encoding and convert to UTF-8.
    // Strip bad control characters.
    $text = iconv("UTF-8", "UTF-8//IGNORE", $text);
    $text = trim(apachesolr_clean_text($text));
    return $text;
  }

  $hash = hash('sha256', file_get_contents($filepath));
  if ($hash === FALSE) {
    return FALSE;
  }

  $cached = db_query("SELECT * FROM {apachesolr_attachments_files} WHERE fid = :fid", array(':fid' => $file['fid']))->fetchAssoc();

  if (!is_null($cached['body']) && ($cached['hash'] == $hash)) {
    // No need to re-extract.
    return $cached['body'];
  }

  if (variable_get('apachesolr_attachments_extract_using', 'tika') == 'tika') {
    $text = apachesolr_attachments_extract_using_tika($filepath);
  }
  else {
    // Extract using Solr.
    try {
      list($text, $metadata) = apachesolr_attachments_extract_using_solr($filepath);
    }
    catch (Exception $e) {
      // Exceptions from Solr may be transient, or indicate a problem with a specific file.
      watchdog('Apache Solr Attachments', "Exception occured sending %filepath to Solr\n!message", array('%filepath' => $file['uri'], '!message' => nl2br(check_plain($e->getMessage()))), WATCHDOG_ERROR);
      return '';
    }
  }
  // Strip bad control characters.
  $text = iconv("UTF-8", "UTF-8//IGNORE", $text);
  $text = trim(apachesolr_clean_text($text));
  // Save the extracted, cleaned text to the DB.
  db_update('apachesolr_attachments_files')->fields(array('hash' => $hash, 'body' => $text))->condition('fid', $file['fid'])->execute();

  return $text;
}

/**
 * For a file path, try to extract text using a local tika jar.
 *
 * @throws Exception
 */
function apachesolr_attachments_extract_using_tika($filepath) {
  $tika_path = realpath(variable_get('apachesolr_attachments_tika_path', ''));

  $tika = realpath($tika_path . '/' . variable_get('apachesolr_attachments_tika_jar', 'tika-0.3.jar'));
  if (!($tika) || !is_file($tika)) {
    throw new Exception(t('Invalid path or filename for tika application jar.'));
  }

  $java = variable_get('apachesolr_attachments_java', 'java');
  // By default force UTF-8 output.
  $java_opts = ' ' . variable_get('apachesolr_attachments_java_opts',  '-Dfile.encoding=UTF8');
  $cmd = escapeshellcmd($java . $java_opts) . ' -cp ' . escapeshellarg($tika_path) . ' -jar ' . escapeshellarg($tika) . ' -t ' . escapeshellarg($filepath);

  // Add a work-around for a MAMP bug + java 1.5.
  if (strpos(ini_get('extension_dir'), 'MAMP/')) {
    $cmd = 'export DYLD_LIBRARY_PATH=""; ' . $cmd;
  }

  return shell_exec($cmd);
}

/**
 * For a file path, try to extract text using Solr 1.4.
 *
 * @throws Exception
 */
function apachesolr_attachments_extract_using_solr($filepath) {
  // Extract using Solr.
  // We allow Solr to throw exceptions - they will be caught
  // by apachesolr.module.
  $solr = apachesolr_get_solr();
  $filename = basename($filepath);
  $params = array(
    'resource.name' => $filename,
    'extractFormat' => 'text', // Matches the -t command for the tika CLI app.
  );
  // Construct a multi-part form-data POST body in $data.
  $boundary = '--' . md5(uniqid(REQUEST_TIME));
  $data = "--{$boundary}\r\n";
  // The 'filename' used here becomes the property name in the response.
  $data .= 'Content-Disposition: form-data; name="file"; filename="extracted"';
  $data .= "\r\nContent-Type: application/octet-stream\r\n\r\n";
  $data .= file_get_contents($filepath);
  $data .= "\r\n--{$boundary}--\r\n";
  $headers = array('Content-Type' => 'multipart/form-data; boundary=' . $boundary);
  $response = $solr->makeServletRequest(EXTRACTING_SERVLET, $params, 'POST', $headers, $data);
  return array($response->extracted, $response->extracted_metadata);
}
