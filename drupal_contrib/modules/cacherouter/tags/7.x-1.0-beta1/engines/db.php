<?php
/**
 * $Id: db.php,v 1.1.4.4 2008/12/27 00:15:40 slantview Exp $
 *
 * @file db.php
 *   Database engine file.
 */
class dbCache extends Cache {  
  function page_fast_cache() {
    return FALSE;
  }
  
  function get($key) {
    global $user;
    
    $cache = parent::get($key);
    if ($cache) {
      return $cache;
    }
    
  $cache = db_query("SELECT data, created, headers, expire, serialized FROM {" . $this->name . "} WHERE cid = :cid", array(':cid' => $this->key($key)))->fetchObject();
    if (isset($cache->data)) {
      if ($cache->serialized) {
        $cache->data = unserialize($cache->data);
      }
	  }
	  parent::set($key, $cache);
	  return $cache;
  }
  
  function set($key, $value, $expire = CACHE_PERMANENT, $headers = NULL) {
    $fields = array(
      'serialized' => 0,
      'created' => REQUEST_TIME,
      'expire' => $expire,
      'headers' => $headers,
      'data' => $value,
      'serialized' => 0,
    );
    if (!is_string($value)) {
      $fields['data'] = serialize($value);
      $fields['serialized'] = 1;
    }

    parent::set($this->key($key), (object)$fields);
    
    db_merge($this->name)
      ->key(array('cid' => $this->key($key)))
      ->fields($fields)
      ->execute();
    
  }

  function delete($key) {
    parent::delete($this->key($key));
    
    if (substr($key, strlen($key) - 1, 1) == '*') {
      $key = $this->key(substr($key, 0, strlen($key) - 1));
      if ($key == '*') {
        db_delete($this->name)->execute();
      }
      else {
        db_delete($this->name)
          ->condition('cid', $key .'%', 'LIKE')
          ->execute();
      }
    }
    else {
      db_delete($this->name)
        ->condition('cid', $this->key($key))
        ->execute();
    }
  }

  function flush() {
    parent::flush();
    db_delete($this->name)
      ->condition('expire', CACHE_PERMANENT, '<>')
      ->condition('expire', REQUEST_TIME, '<')
      ->execute();
  }
}