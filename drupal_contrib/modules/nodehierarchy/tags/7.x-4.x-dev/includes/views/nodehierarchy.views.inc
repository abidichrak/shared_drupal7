<?php
/**
 * @file
 *   Implementations of Views 2 Hooks for nodehierarchy_views module
 */

/**
 * Implements hook_views_handlers().
 */
function nodehierarchy_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'nodehierarchy') . '/includes/views',
    ),
    'handlers' => array(
      'views_handler_field_nodehierarchy_actions' => array(
        'parent' => 'views_handler_field',
      ),
      'views_handler_field_nodehierarchy_parent' => array(
        'parent' => 'views_handler_field',
      ),
      'views_handler_argument_nodehierarchy_ancestor' => array(
        'parent' => 'views_handler_argument',
      ),
    ),
  );
}

/**
 * Implements hook_views_data().
 */
function nodehierarchy_views_data() {

  // Basic table information.
  $data['nodehierarchy']['table']['group']  = t('Node Hierarchy');
  $data['nodehierarchy']['table']['join'] = array(
    'node' => array(
      'left_field' => 'nid',
      'field' => 'cnid',
    ),
  );

  $data['nodehierarchy']['cweight'] = array(
    'title' => t('Child Weight'),
    'help' => t('The sort order of the child node.'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['nodehierarchy']['pnid'] = array(
    'title' => t('Parent Node ID'),
    'help' => t('The parent id of the node.'),
    // Information for accepting a parent as an argument.
    'argument' => array(
      'field' => 'pnid',
      'handler' => 'views_handler_argument_node_nid',
      'click sortable' => TRUE,
    ),
    'relationship' => array(
      'base' => 'node',
      'base field' => 'nid',
      'field' => 'pnid',
      'handler' => 'views_handler_relationship',
      'label' => t('Parent Node'),
    ),
  );

  return $data;
}
