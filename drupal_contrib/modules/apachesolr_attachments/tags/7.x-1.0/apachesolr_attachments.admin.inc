<?php

/**
 * @file
 * Provides a file attachment search implementation for use with the Apache Solr module
 */

/**
 * Menu callback: Apache Solr Attachments settings tab.
 */
function apachesolr_attachments_admin_page() {
  $output['apachesolr_attachments_settings'] = drupal_get_form('apachesolr_attachments_settings');
  $output['apachesolr_attachments_index_action_form'] = drupal_get_form('apachesolr_attachments_index_action_form');
  return $output;
}

/**
 * Displays the Attachment Settings Form.
 *
 * @see apachesolr_attachments_settings_validate()
 * @see apachesolr_attachments_settings_submit()
 */
function apachesolr_attachments_settings($form) {
  $default = implode(' ', apachesolr_attachments_default_excluded());
  $form['apachesolr_attachments_excluded_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Excluded file extensions'),
    '#default_value' => variable_get('apachesolr_attachments_excluded_extensions', $default),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('File extensions that are excluded from indexing. Separate extensions with a space and do not include the leading dot. Extensions are internally mapped to a MIME type, so it is not necessary to put variations that map to the same type (e.g. tif is sufficient for tif and tiff)'),
  );
  $form['apachesolr_attachments_exclude_types'] = array(
    '#type' => 'radios',
    '#title' => t('Exclude files attached to a node of a type excluded by Apache Solr Search'),
    '#options' => array(
      '0' => t('No'),
      '1' => t('Yes'),
    ),
    '#default_value' => variable_get('apachesolr_attachments_exclude_types', 1),
  );
  $form['apachesolr_attachments_extract_using'] = array(
    '#type' => 'radios',
    '#title' => t('Extract using'),
    '#options' => array(
      'tika' => t('Tika (local java application)'),
      'solr' => t('Solr (remote server)'),
    ),
    '#description' => t("Extraction will be faster if run locally using tika."),
    '#default_value' => variable_get('apachesolr_attachments_extract_using', 'tika'),
  );
  $form['apachesolr_attachments_tika_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Tika directory path'),
    '#size' => 80,
    '#maxlength' => 100,
    '#description' => t("The full path to the tika directory. All library jars must be in the same directory. If on Windows, use forward slashes in the path."),
    '#default_value' => variable_get('apachesolr_attachments_tika_path', ''),
  );
  $form['apachesolr_attachments_tika_jar'] = array(
    '#type' => 'textfield',
    '#title' => t('Tika jar file'),
    '#size' => 20,
    '#description' => t("The name of the tika CLI application jar file, e.g. tika-0.3.jar or tika-app-0.4.jar."),
    '#default_value' => variable_get('apachesolr_attachments_tika_jar', 'tika-0.3.jar'),
  );
  $form['apachesolr_attachments-cron-settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Cron settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['apachesolr_attachments-cron-settings']['apachesolr_attachments_cron_limit'] = array(
    '#type' => 'select',
    '#title' => t('Maximum number of nodes to examine'),
    '#default_value' => variable_get('apachesolr_attachments_cron_limit', 100),
    '#options' => drupal_map_assoc(array(10, 20, 50, 100, 200)),
  );
  $form['apachesolr_attachments-cron-settings']['apachesolr_attachments_cron_time_limit'] = array(
    '#type' => 'select',
    '#title' => t('Maximum time to expend (sec)'),
    '#default_value' => variable_get('apachesolr_attachments_cron_time_limit', 15),
    '#options' => drupal_map_assoc(array(5, 10, 15, 20, 25, 30, 45, 60)),
  );

  $form = system_settings_form($form);
  $form['#validate'][] = 'apachesolr_attachments_settings_validate';
  $form['#submit'][] = 'apachesolr_attachments_settings_submit';
  return $form;
}

/**
 * Form validation for the Apache Solr Attachments settings form.
 *
 * @see apachesolr_attachments_settings()
 */
function apachesolr_attachments_settings_validate($form, &$form_state) {
  if ($form_state['values']['apachesolr_attachments_extract_using'] == 'tika') {
    $path = realpath($form_state['values']['apachesolr_attachments_tika_path']);
    if (!file_exists($path . '/' . $form_state['values']['apachesolr_attachments_tika_jar'])) {
      form_set_error('apachesolr_attachments_tika_path', t('Tika jar file not found at this path.'));
    }
  }
}

/**
 * Form submit handler for the settings Form
 *
 * @see apachesolr_attachments_settings()
 */
function apachesolr_attachments_settings_submit($form, &$form_state) {
  // Delete this so it's rebuilt.
  variable_del('apachesolr_attachments_excluded_mime');
  drupal_set_message(t('If you changed the allowed file extensions, you may need to delete and re-index all attachments.'));
}

/**
 * Create a form for deleting the contents of the Solr index.
 */
function apachesolr_attachments_index_action_form() {
  $form = array();

  $form['action'] = array(
    '#type' => 'fieldset',
    '#title' => t('Index Actions'),
  );
  // Jump through some forms hoops to get a description under each radio button.
  $actions = array(
    'reindex' => array(
      'title' => t('Re-index all files'),
      'description' => t('Re-indexing will add all file text to the index again (overwriting the index), but existing text in the index will remain searchable.'),
    ),
    'delete' => array(
      'title' => t('Delete files from index'),
      'description' => t('Deletes all of the files in the Solr index and reindexes them. This may be needed if you have changed the allowed file extensions, if your index is corrupt, or if you have installed a new schema.xml.'),
    ),
    'clear-cache' => array(
      'title' => t('Delete cached file text'),
      'description' => t('Deletes the local cache of extracted text from files. This will cause slower performance when reindexing since text must be re-extracted.'),
    ),
  );
  foreach ($actions as $key => $action) {
    // Generate the parents as the autogenerator does, so we will have a
    // unique id for each radio button.
    $form['action'][$key] = array(
      '#type' => 'radio',
      '#title' => $action['title'],
      '#default_value' => 'remaining',
      '#return_value' => $key,
      '#parents' => array('action'),
      '#description' => $action['description'],
      '#id' => drupal_clean_css_identifier('edit-'. implode('-', array('action', $key))),
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Begin'),
    '#submit' => array('apachesolr_attachments_index_action_form_submit'),
  );

  return $form;
}

/**
 * Submit function for the index action form.
 */
function apachesolr_attachments_index_action_form_submit($form, &$form_state) {
  switch ($form_state['values']['action']) {
    case 'reindex':
      $form_state['redirect'] = 'admin/config/search/apachesolr/attachments/confirm/reindex';
      break;

    case 'delete':
      $form_state['redirect'] = 'admin/config/search/apachesolr/attachments/confirm/delete';
      break;

    case 'clear-cache':
      $form_state['redirect'] = 'admin/config/search/apachesolr/attachments/confirm/clear-cache';
      break;
  }
}

/**
 * Index confirmation form
 *
 * @see apachesolr_attachments_confirm_submit()
 */
function apachesolr_attachments_confirm($form, $form_state, $operation) {
  $form = array();
  $form['operation'] = array(
    '#type' => 'value',
    '#value' => $operation,
  );
  switch ($operation) {
    case 'reindex':
      $text = t('Are you sure you want to re-index the text of all file attachments?');
      break;
    case 'delete':
      $text = t('Are you sure you want to delete and re-index the text of all file attachments?');
      break;
    case 'clear-cache':
      $text = t('Are you sure you want to delete the cache of extracted text from file attachments?');
      break;
  }
  return confirm_form($form, $text, 'admin/config/search/apachesolr/attachments', NULL, t('Confirm'), t('Cancel'));
}

/**
 * Form submit handler for the index confirmation form
 *
 * @see apachesolr_attachments_confirm()
 */
function apachesolr_attachments_confirm_submit($form, &$form_state) {
  switch ($form_state['values']['operation']) {
    case 'delete':
      if (apachesolr_attachments_delete_index()) {
        drupal_set_message(t('File text has been deleted from the Apache Solr index. You must now <a href="!url">run cron</a> until all files have been re-indexed.', array('!url' => url('admin/reports/status/run-cron', array('query' => array('destination' => 'admin/config/search/apachesolr/index'))))));
      }
      else {
        if (module_exists('dblog')) {
          drupal_set_message(t('Could not delete file text from the Apache Solr index. Check <a href="!url">recent log messages</a>.', array('!url' => url('admin/reports/dblog'))));
        }
        else {
          drupal_set_message(t('Could not delete file text from the Apache Solr index.'));
        }
      }
      break;
    case 'reindex':
      apachesolr_clear_last_index('apachesolr_attachments');
      drupal_set_message(t('All file attachments will be re-indexed.'));
      break;
    case 'clear-cache':
      db_delete('apachesolr_attachments_files')->condition('removed', 0)->execute();
      drupal_set_message(t('The local cache of extracted text has been deleted.'));
      break;
  }
  $form_state['redirect'] = 'admin/config/search/apachesolr/attachments';
}

/**
 * @see apachesolr_delete_index()
 */
function apachesolr_attachments_delete_index() {
  try {
    $solr = apachesolr_get_solr();
    $solr->deleteByQuery("entity_type:file AND hash:" . apachesolr_site_hash());
    $solr->commit();
    apachesolr_index_set_last_updated(REQUEST_TIME);
    apachesolr_clear_last_index('apachesolr_attachments');
    return TRUE;
  }
  catch (Exception $e) {
    watchdog('Apache Solr Attachments', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
  }
  return FALSE;
}
