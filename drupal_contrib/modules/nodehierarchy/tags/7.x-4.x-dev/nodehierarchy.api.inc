<?php
/**
 * @file
 *
 * API functions for Node Hierarchy
 */

/**
 * Get the parent nodes for the given node.
 */
function nodehierarchy_get_node_parent_nids($node, $limit = NULL) {
  $out = array();
  foreach (nodehierarchy_get_node_parents($node, $limit) as $parent) {
    // This may be an array after a node has been submitted.
    $parent = (object)$parent;
    $out[] = $parent->pnid;
  }
  return $out;
}

/**
 * Get the parent nodes for the given node.
 */
function nodehierarchy_get_node_parent_nodes($node) {
  return node_load_multiple(nodehierarchy_get_node_parent_nids($node));
}

/**
 * Get the primary parent nid for the given node.
 */
function nodehierarchy_get_node_parent_primary($node) {
  $parents = nodehierarchy_get_node_parents($node, 1);
  return current($parents);
}

/**
 * Get the primary parent nid for the given node.
 */
function nodehierarchy_get_node_parent_primary_nid($node) {
  $nodes = nodehierarchy_get_node_parent_nids($node, 1);
  return current($nodes);
}


/**
 * Get the primary parent node for the given node.
 */
function nodehierarchy_get_node_parent_primary_node($node) {
  if ($pnid = nodehierarchy_get_node_parent_primary_nid($node)) {
    return node_load($pnid);
  }
}

/**
 * Get the parent nodes for the given node.
 */
function nodehierarchy_get_node_primary_ancestor_nodes($node) {
  return node_load_multiple(nodehierarchy_get_node_primary_ancestor_nids($node));
}

/**
 * Get the ancestor nodes for the given node.
 * 
 * @TODO: make this more efficient by implementing a materialized path or similar.
 */
function nodehierarchy_get_node_primary_ancestor_nids($node) {
  $out = array();
  
  if ($parent = nodehierarchy_get_node_parent_primary_nid($node)) {
    $out = nodehierarchy_get_node_primary_ancestor_nids($parent);
    $out[] = $parent;
  }

  return $out;
}

/**
 * Get the ALL ancestor nodes for the given node.
 * 
 * @TODO: make this more efficient by implementing a materialized path or similar.
 */
function nodehierarchy_get_node_ancestor_nids($node) {
  $out = array();
  
  foreach (nodehierarchy_get_node_parent_nids($node) as $parent) {
    $out = nodehierarchy_get_node_ancestor_nids($parent);
    $out[] = $parent;
  }

  return $out;
}


/**
 * Get the children of the given node.
 */
function nodehierarchy_get_node_children($node, $limit = FALSE) {
  $pnid = $node;
  if (is_object($node)) {
    $pnid = $node->nid;
  }

  $query = db_select('nodehierarchy', 'nh')
            ->fields('nh')
            ->fields('n', array('title'))
            ->where('pnid = :pnid', array(':pnid' => $pnid))
            ->orderBy('cweight', 'ASC');

  $query->leftJoin('node', 'n', 'nh.cnid = n.nid');

  if ($limit) {
    $query->range(0, $limit);    
  }

  $result = $query->execute()->fetchAll();
  $children = array();
  foreach ($result as $item) {
    $children[] = $item;
  }
  return $children;
}

/**
 * Count the children of the given node.
 */
function nodehierarchy_get_node_children_count($node) {
  $pnid = $node;
  if (is_object($node)) {
    $pnid = $node->nid;
  }

  return (int)db_query("SELECT count(*) FROM {nodehierarchy} WHERE pnid = :pnid", array(':pnid' => $pnid))->fetchField();
}

/**
 * Count the descendants of the given node.
 */
function nodehierarchy_get_node_descendant_count($pnid) {
  // @TODO: implement this.
  return 0;
}

/**
 * Get all the parents for the given node.
 */
function nodehierarchy_get_node_parents($node, $limit = NULL) {
  $cnid = $node;

  // If a node object was passed, then the parents may already have been loaded.
  if (is_object($node)) {
    if (isset($node->nodehierarchy_parents)) {
      return $node->nodehierarchy_parents;
    }
    $cnid = $node->nid;
  }

  $out = array();

  $query = db_select('nodehierarchy', 'nh')
            ->fields('nh')
            ->where('cnid = :cnid', array(':cnid' => $cnid))
            ->orderBy('pweight', 'ASC');

  if ($limit) {
    $query->range(0, $limit);    
  }

  $result = $query->execute()->fetchAll();

  foreach ($result as $item) {
    $out[] = $item;
  }
  return $out;
}

