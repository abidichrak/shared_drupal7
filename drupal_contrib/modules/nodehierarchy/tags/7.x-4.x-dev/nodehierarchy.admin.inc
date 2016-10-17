<?php
/**
 * @file
 *
 * Admin functions for Node Hierarchy
 */


/**
 * Set a default parent when the node is being prepared for the edit screen.
 */
function nodehierarchy_prepare_node($node) {
  // Load the parents if that hasn't been done before.
  if (!isset($node->nodehierarchy_parents) && !empty($node->nid)) {
    $node->nodehierarchy_parents = nodehierarchy_get_node_parents($node->nid);
  }
  // Cannot use module_invoke_all because it doesn't support references.
  foreach (module_implements('nodehierarchy_default_parents') as $module) {
    $function = $module . '_nodehierarchy_default_parents';
    $function($node);
  }
}

/**
 * Update a node's parent and create menus etc.
 */
function nodehierarchy_update_node(&$node) {
  global $user;
  if (user_access('edit all node parents') || ($node->uid == $user->uid && user_access('edit own node parents'))) {
    _nodehierarchy_save_node($node);
  }
}

/**
 * Insert a node. Create parents and menus etc.
 */
function nodehierarchy_insert_node(&$node) {
  if (user_access('create child nodes')) {
    _nodehierarchy_save_node($node);
  }
}

/**
 * Add Node Hierarchy settings to the node form.
 */
function nodehierarchy_alter_node_form(&$form, &$form_state, $form_id) {
  $type = isset($form['type']) && isset($form['#node']) ? $form['type']['#value'] : '';
  $node = isset($form['#node']) ? $form['#node'] : NULL;
  if (isset($node->nid)) {
    nodehierarchy_set_breadcrumbs($node, TRUE);
  }
  $hierarchy_form = module_invoke_all('nodehierarchy_node_form', $node, $form, $form_state);

  // If the current user has no nodehierarchy perms, don't show the form.
  $access = FALSE;
  foreach (nodehierarchy_permission() as $perm => $info) {
    if (user_access($perm)) {
      $access = TRUE;
      break;
    }
  }

  if ($hierarchy_form) {
    // Add the js to show the summary.
    drupal_add_js(drupal_get_path('module', 'nodehierarchy') . '/nodehierarchy.edit.js');
    $weight = function_exists('content_extra_field_weight') ? content_extra_field_weight($type, 'nodehierarchy') : 10;
    $form['nodehierarchy'] = array_merge(
      array(
      '#type' => 'fieldset',
      '#title' => t('Node Hierarchy'),
      '#group' => 'additional_settings',
      '#weight' => $weight,
      '#access' => $access,
      '#attributes' => array(
        'class' => array('nodehierarchy-form'),
      ),
    ),
    $hierarchy_form);
  }
}

/**
 * Add Node Hierarchy settings to the node type setting form.
 */
function nodehierarchy_alter_node_type_form(&$form, &$form_state, $form_id) {
  $type = $form['old_type']['#value'];

  $form['nodehierarchy'] = array(
    '#type' => 'fieldset',
    '#group' => 'additional_settings',
    '#title' => t('Node Hierarchy'),
    '#weight' => 10,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#attached' => array(
      'js' => array(drupal_get_path('module', 'nodehierarchy') . '/nodehierarchy.edit.js'),
    ),
  );

  $form['nodehierarchy'] += _nodehierarchy_get_node_type_settings_form($type);
}

/**
 * Get the nodehierarchy setting form for a particular node type.
 */
function _nodehierarchy_get_node_type_settings_form($key, $append_key = FALSE) {
  $form              = array();
  $form['nh_allowchild'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Allowed child node types'),
    '#options' => node_type_get_names(),
    '#default_value' => nodehierarchy_get_allowed_child_types($key),
    '#description' => t('Node types which can be created as child nodes of this node type.'),
  );
  $form['nh_defaultparent'] = _nodehierarchy_get_parent_selector($key, variable_get('nh_defaultparent_' . $key, 0));
  $form['nh_defaultparent']['#title'] = t('Default Parent');

  $form += module_invoke_all('nodehierarchy_node_type_settings_form', $key);

  // If we need to append the node type key to the form elements, we do so.
  if ($append_key) {
    // Appending the key does not work recursively, so fieldsets etc. are not supported.
    foreach (element_children($form) as $form_key) {
      $form[$form_key . '_' . $key] = $form[$form_key];
      unset($form[$form_key]);
    }
  }
  return $form;
}



 /**
 * Add Node Hierarchy delete options to the node delete confirm form.
 */
function nodehierarchy_alter_node_delete_confirm_form(&$form, &$form_state, $form_id) {
  // TODO: Fix the descendant count code to deal with multiparent situations.
  if ($count = nodehierarchy_get_node_children_count($form['nid']['#value'])) {
    $items = array();
    foreach (nodehierarchy_get_node_children($form['nid']['#value'], 10) as $child) {
      $items[] = check_plain($child->title);
    }
    if ($count > 10) {
      $items[] = l(t('See all !count children', array('!count' => $count)), 'node/' . $form['nid']['#value'] . '/children');
    }
    $list = theme('item_list', array('items' => $items));
    $description = format_plural($count, 'This node has @count child. Check this box to delete it and all of its descendants as well.', 'This node has @count children. Check this box to delete them and all of their descendants as well.' );
    $description .= t('<p>These children and their decendants will be deleted:!list<p>', array('!list' => $list));
    $form['nodehierarchy_delete_children'] = array(
      '#type' => 'checkbox',
      '#title' => t('Delete descendants'),
      '#description' => $description,
    );
    array_unshift($form['#submit'], 'nodehierarchy_node_delete_form_submit');
    $form['actions']['#weight'] = 1;
  }
}

/**
 * Submit function for the node delete confirm form.
 */
function nodehierarchy_node_delete_form_submit($form, $form_state) {
  $form_values = $form_state['values'];
  if ($form_values['confirm'] && $form_values['nodehierarchy_delete_children']) {
    nodehierarchy_delete_descendants($form_values['nid']);
  }
}

/**
 * Get the node edit form for nodehierarchy.
 */
function _nodehierarchy_nodehierarchy_node_form($node, $form, &$form_state) {
  global $user;

  $form = array();
  // If this node type can be a child.
  if (nodehierarchy_node_can_be_child($node) || nodehierarchy_node_can_be_parent($node)) {
    // if the current user can edit the current node's hierarchy settings (or create new children)
    $can_set_parent =
        user_access('edit all node parents') ||
        (empty($node->nid) && user_access('create child nodes')) ||
        ($node->uid == $user->uid && user_access('edit own node parents'));

    if ($can_set_parent) {
      $form['nodehierarchy'] = array('#tree' => FALSE);
      $form['nodehierarchy']['nodehierarchy_parents'] = array('#tree' => TRUE);

      foreach ((array)$node->nodehierarchy_parents as $key => $parent) {
        $form['nodehierarchy']['nodehierarchy_parents'][$key] = _nodehierarchy_node_parent_form_items($node, $parent, $key);
        drupal_alter('nodehierarchy_node_parent_form_items', $form['nodehierarchy']['nodehierarchy_parents'][$key], $node, $parent);
      }
      drupal_alter('nodehierarchy_node_parent_form_items_wrapper', $form['nodehierarchy'], $form_state, $node);
    }
  }
  return $form;
}


/**
 * Get the parent and menu setting for items for a given parent menu_link.
 */
function _nodehierarchy_node_parent_form_items($node, $parent, $key) {
  // Wrap the item in a div for js purposes
  $item = array(
    '#type' => 'fieldset',
    '#title' => t('Parent'),
    '#tree' => TRUE,
    '#prefix' => '<div class="nodehierarchy-parent">',
    '#suffix' => '</div>',
  );

  // If a node can be a child of another add a selector to pick the parent. Otherwise set the parent to 0.
  if (nodehierarchy_node_can_be_child($node)) {
    $item['pnid'] = _nodehierarchy_get_parent_selector($node->type, empty($parent->pnid) ? null : $parent->pnid, empty($node->nid) ? null : $node->nid);
    $item['pnid']['#weight'] = -1;
  }
  else {
    $item['pnid'] = array(
      '#type' => 'value',
      '#value' => 0,
    );
  }

  $item['nhid'] = array(
    '#type' => 'value',
    '#value' => isset($parent->nhid) ? $parent->nhid : NULL,
  );
  $item['cweight'] = array(
    '#type' => 'value',
    '#value' => isset($parent->cweight) ? $parent->cweight : NULL,
  );
  $item['pweight'] = array(
    '#type' => 'value',
    '#value' => isset($parent->pweight) ? $parent->pweight : NULL,
  );

  if (!empty($parent->nhid)) {
    $item['remove'] = array(
      '#type' => 'checkbox',
      '#title' => t('Remove this parent'),
      '#weight' => 100,
    );
  }


  return $item;
}


/**
 * Helper function generates admin settings page.
 */
function nodehierarchy_admin_settings($form, &$form_state) {
  $form = array();

  // Individual type settings.
  $form['nodehierarchy_types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node Type Settings'),
    '#description' => t('Settings for individual node types. These can also be set in the !ct section.', array("!ct" => l(t("Content Types"), "admin/structure/types"))),
  );
  foreach (node_type_get_types() as $key => $type) {
    // Individual type settings.
    $form['nodehierarchy_types'][$key] = array(
      '#type' => 'fieldset',
      '#title' => $type->name,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['nodehierarchy_types'][$key] += _nodehierarchy_get_node_type_settings_form($key, TRUE);
  }

  // Menu generation.
  if (function_exists('menu_parent_options')) {
    $form['nodehierarchy_menu'] = array(
      '#type' => 'fieldset',
      '#title' => t('Node Hierarchy Menu Generation'),
    );
    $form['nodehierarchy_menu']['nodehierarchy_default_menu_name'] = array(
      '#type' => 'select',
      '#title' => t('Default parent menu'),
      '#options' => menu_get_menus(),
      '#default_value' => variable_get('nodehierarchy_default_menu_name', 'navigation'),
      '#description' => t('If a menu is created for a node with no parent the new menu item will appear in this menu.'),
    );

    $form['nodehierarchy_menu']['nodehierarchy_menu_module_edit'] = array(
      '#type' => 'checkbox',
      '#title' => t('Always show hidden Node Hierarchy menu items on the menu overview forms.'),
      '#default_value' => variable_get('nodehierarchy_menu_module_edit', FALSE),
      '#description' => t('Allow disabled nodehierarchy menu items to be edited with regular menu items in the menu overview screen. Turn this off if large Node Hierarchy menus are causing memory errors on menu edit screens.'),
    );
  }

  return system_settings_form($form);
}

/**
 * Display the children tab.
 */
function nodehierarchy_view_children($node) {
  drupal_set_title(t('Children of %t', array('%t' => $node->title)), PASS_THROUGH);
  nodehierarchy_set_breadcrumbs($node, TRUE);
  $out = drupal_get_form('nodehierarchy_children_form', $node);
  $out[] = theme('nodehierarchy_new_child_links', array('node' => $node));
  return $out;
}

/**
 * Built the children tab form.
 */
function nodehierarchy_children_form($form, &$form_state, $node) {
  $form = array();
  // $children_links = _nodehierarchy_get_children_menu_links($node->nid, FALSE);
  $children = nodehierarchy_get_node_children($node, FALSE);

  $form['children'] = array('#tree' => TRUE);
  $type_names = node_type_get_names();

  // Find the maximum weight.
  $delta = count($children);
  foreach ($children as $child) {
    $delta = max($delta, $child->cweight);
  }

  foreach ($children as $child) {
    if ($node = node_load($child->cnid)) {
      $child_item = array();
      $child_item['child']  = array(
        '#type' => 'value',
        '#value' => $child,
      );
      $child_item['node']       = array(
        '#type' => 'value',
        '#value' => $node,
      );
      $child_item['title']      = array('#type' => 'markup', '#markup' => l($node->title, 'node/' . $node->nid));
      $child_item['type']       = array('#type' => 'markup', '#markup' => $type_names[$node->type]);

      $child_item['cweight']    = array(
        '#type' => 'weight',
        '#delta' => $delta,
        '#default_value' => isset($form_state[$child->nhid]['cweight']) ? $form_state[$child->nhid]['cweight'] : $child->cweight,
      );

      $form['children'][$child->nhid] = $child_item;
    }
  }

  if (element_children($form['children'])) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save child order'),
    );
  }
  else {
    $form['no_children'] = array('#type' => 'markup', '#markup' => t('This node has no children.'));
  }
  return $form;
}

/**
 * Submit the children tab form.
 */
function nodehierarchy_children_form_submit($form, &$form_state) {
  $updated_items = array();
  foreach (element_children($form['children']) as $nhid) {
    if (isset($form['children'][$nhid]['child'])) {
      $element = $form['children'][$nhid];
      $child = $element['child']['#value'];
      if ($element['cweight']['#value'] != $child->cweight) {
        $child->cweight = $element['cweight']['#value'];
        $updated_items[$nhid] = $child;
      }
    }
  }

  // Save all our changed items to the database.
  foreach ($updated_items as $item) {
    // Skip the save function wrapper as we don't need child weights recalculated.
    _nodehierarchy_record_write($item);
  }
  module_invoke_all('nodehierarchy_reorder_children', $updated_items);
}

/**
 * Display the children tab form.
 */
function theme_nodehierarchy_children_form($variables) {
  $form = $variables['form'];
  drupal_add_tabledrag('children-list', 'order', 'sibling', 'cweight');

  $colspan = module_exists('nodeaccess') ? '4' : '3';

  $header = array(
    t('Title'),
    t('Type'),
    t('Weight'),
    array(
      'data' => t('Operations'),
      'colspan' => $colspan,
    ),
  );

  $rows = array();
  foreach (element_children($form['children']) as $nhid) {
    $element = &$form['children'][$nhid];

    // Add special classes to be used for tabledrag.js.
    $element['cweight']['#attributes']['class'] = array('cweight');

    $node = $element['node']['#value'];
    $row = array();
    $row[] = drupal_render($element['title']);
    $row[] = drupal_render($element['type']);
    $row[] = drupal_render($element['cweight']);
    $row[] = node_access('update', $node) ? l(t('edit'), 'node/' . $node->nid . '/edit', array('query' => drupal_get_destination())) : '';
    $row[] = node_access('delete', $node) ? l(t('delete'), 'node/' . $node->nid . '/delete', array('query' => drupal_get_destination())) : '';

    $row[] = nodehierarchy_children_tab_access($node) ? l(t('children'), 'node/' . $node->nid . '/children') : '';
    if (module_exists('nodeaccess')) {
      $row[] = nodeaccess_access('grant', $node) ? l(t('grant'), 'node/' . $node->nid . '/grant') : '';
    }

    $row = array_merge(array('data' => $row), $element['#attributes']);
    $row['class'][] = 'draggable';
    $rows[] = $row;
  }
  $output = '';
  if ($rows) {
    $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'children-list')));
  }
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Theme the parent selector pulldown, allowing for disabled options.
 */
function theme_nodehierarchy_parent_selector($variables) {
  $element = $variables['element'];
  element_set_attributes($element, array('id', 'name', 'size'));
  _form_set_class($element, array('form-select'));

  // Assemble the options.
  $options = '';
  foreach ($element['#options'] as $key => $option) {
    $attributes = '';
    // If the option represents a parent item (and not just the none option).
    if (!empty($element['#items'][$key]) && $item = $element['#items'][$key]) {
      if ($element['#value'] == $key) {
        $attributes .= ' selected="selected"';
      }
      if ($item->disabled) {
        $attributes .= ' disabled="disabled"';
        $option .= '*';
        $element['#description'] = t('Nodes marked with a * cannot be a parent for this node because they are not an allowed parent type.');
        if (user_access('administer hierarchy')) {
          $element['#description'] .= t(' To allow these nodes to be parents of this node, change the setting for that node type in the !settings', array('!settings' => l(t('Node Hierarchy settings'), 'admin/config/nodehierarchy')));
        }
      }
    }
    $options .= '<option value="' . check_plain($key) . '"' . $attributes . '>' . $option . '</option>';
  }
  return '<select' . drupal_attributes($element['#attributes']) . '>' . $options . '</select>';
}

/**
 * Validate the parent node selector to make sure the parent is legal.
 */
function nodehierarchy_parent_selector_validate($element, &$form_state) {
  $selection = @$element['#items'][$element['#value']];
  if (is_array($selection) && !empty($selection['disabled'])) {
    form_error($element, t('You have selected a parent node which cannot be a parent of this node type.'));

    if (!empty($selection['nid']) && !empty($form_state['values']['nid']) && in_array($form_state['values']['nid'], nodehierarchy_get_node_ancestor_nids($selection['nid']))) {
      form_error($element, t('The parent of this node can not be itself or any if its decendants.'));
    }
  }
}


/**
 * Return a list of menu items that are valid possible parents for the given node.
 */
function _nodehierarchy_parent_options($child_type, $exclude = NULL) {
  $options = &drupal_static(__FUNCTION__);

  // If these options have already been generated, then return that saved version.
  if (isset($options[$child_type][$exclude])) {
    return $options[$child_type][$exclude];
  }

  // Get all the possible parents.
  // @TODO: implement this if needed. Early testing shows a negligible difference.
  // $parent_tree = array();
  // $cache = cache_get('nodehierarchy_parent_options');
  // if (isset($cache->data)) {
  //   $parent_tree = $cache->data;
  // }
  // else 
  {
    $types = nodehierarchy_get_allowed_parent_types();
    $parent_types = nodehierarchy_get_allowed_parent_types($child_type);

    // Build the whole tree.
    $parent_tree = _nodehierarchy_nodes_by_type($types);
    $parent_tree = _nodehierarchy_build_tree($parent_tree);

    // Remove or disable items that can't be parents.
    $parent_tree = _nodehierarchy_tree_disable_types($parent_tree, $parent_types);
    
    // cache_set('nodehierarchy_parent_options', $parent_tree);
  }

  // Remove items which the user does not have permission to.
  $parent_tree = _nodehierarchy_tree_disable_noaccess($parent_tree);

  // Remove the excluded item(s). This prevents a child being assigned as it's own parent.
  $out = _nodehierarchy_tree_remove_nid($parent_tree, $exclude);

  // Convert the tree to a flattened list (with depth)
  $out = _nodehierarchy_flatten_tree($out);

  // Static caching to prevent these options being built more than once.
  $options[$child_type][$exclude] = $out;

  return $out;
}

/**
 * Clear the parent options cache.
 */
function _nodehierarchy_parent_options_cache_clear() {
  cache_lear_all('nodehierarchy_parent_options');
}

/**
 * Get a tree of nodes of the given type.
 */
function _nodehierarchy_nodes_by_type($types, $pnid = NULL, $depth = 0) {
  $out = array();
  
  if ($types) {    
    $query = db_select('node', 'n')
              ->fields('n', array('nid', 'type', 'title', 'uid', 'status'))
              ->fields('nh', array('cweight', 'pnid'))
              ->condition('n.type', $types, 'IN')
              ->orderBy('nh.cweight', 'ASC');
    $query->leftJoin('nodehierarchy', 'nh', 'nh.cnid = n.nid');

    $result = $query->execute();
    foreach ($result as $item) {
      $out[$item->nid] = $item;
    }
  }

  return $out;
}

/**
 * Get a tree of nodes of the given type.
 */
function _nodehierarchy_build_tree($nodes) {
  foreach ($nodes as $node) {
    $node->is_child = FALSE;
    $node->disabled = FALSE;
    if (!empty($node->pnid)) {
      if(isset($nodes[$node->pnid])) {
        $node->is_child = TRUE;
        $nodes[$node->pnid]->children[$node->nid] = &$nodes[$node->nid];
      }
    }
  }
  foreach ($nodes as $nid => $node) {
    if ($node->is_child) {
      unset($nodes[$nid]);
    }
  }
  return $nodes;
}

/**
 * Flatten the tree of nodes.
 */
function _nodehierarchy_flatten_tree($nodes, $depth = 1) {
  $out = $children = array();
  foreach ($nodes as $nid => $node) {
    $node->depth = $depth;
    $children = array();
    if (!empty($node->children)) {
      $children = _nodehierarchy_flatten_tree($node->children, $depth + 1);
    }

    // Only output this option if there are non-disabled chidren.
    if (!$node->disabled || $children) {
      $out[$nid] = $node;
      $out += $children;
    }
  }
  return $out;
}

/**
 * Mark nodes that are not of the given types as disabled.
 */
function _nodehierarchy_tree_disable_types($nodes, $allowed_types) {
  foreach ($nodes as $nid => $node) {
    if (!in_array($node->type, $allowed_types)) {
      $nodes[$nid]->disabled = TRUE;
    }
    if (!empty($node->children)) {
      $nodes[$nid]->children = _nodehierarchy_tree_disable_types($node->children, $allowed_types);
    }
  }
  return $nodes;
}

/**
 * Mark nodes which the user does not have edit access to as disabled.
 */
function _nodehierarchy_tree_disable_noaccess($nodes) {
  if (!user_access('create child of any parent')) {
    foreach ($nodes as $nid => $node) {
      $nodes[$nid]->disabled = $nodes[$nid]->disabled || !node_access('update', $node);

      if (!empty($node->children)) {
        $nodes[$nid]->children = _nodehierarchy_tree_disable_noaccess($node->children);
      }
    }
  }
  return $nodes;
}

/**
 * Mark nodes with the given node and it's decendents as disabled.
 */
function _nodehierarchy_tree_remove_nid($nodes, $exclude) {
  foreach ($nodes as $nid => $node) {
    if ($nid == $exclude) {
      unset($nodes[$nid]);
    }
    else if (!empty($node->children)) {
      $nodes[$nid]->children = _nodehierarchy_tree_remove_nid($node->children, $exclude);
    }
  }
  return $nodes;
}



/**
 * Get the title of the given item to display in a pulldown.
 */
function _nodehierarchy_parent_option_title($item) {
  return str_repeat('--', $item->depth - 1) . ' ' . truncate_utf8($item->title, 60, TRUE, FALSE);
}

/**
 * Display links to create new children nodes of the given node
 */
function nodehierarchy_new_child_links($node) {
  return theme('nodehierarchy_new_child_links', array('node' => $node));
}

/**
 * Display links to create new children nodes of the given node
 */
function theme_nodehierarchy_new_child_links($variables) {
  $node = $variables['node'];
  $out = array();
  $create_links = array();

  if (user_access('create child nodes') && (user_access('create child of any parent') || node_access('update', $node))) {
    foreach (nodehierarchy_get_allowed_child_types($node->type) as $key) {
      if (node_access('create', $key)) {
        $type_name = node_type_get_name($key);
        $destination = (array)drupal_get_destination() + array('parent' => $node->nid);
        $key = str_replace('_', '-', $key);
        $title = t('Add a new %s.', array('%s' => $type_name));
        $create_links[] = l($type_name, "node/add/$key", array('query' => $destination, 'attributes' => array('title' => $title)));
      }
    }
    if ($create_links) {
      $out[] = array('#children' => '<div class="newchild">' . t("Create new child !s", array('!s' => implode(" | ", $create_links))) . '</div>');
    }
  }
  return $out;
}


/**
 * Delete the nodehierarchy information when a node is deleted.
 */
function nodehierarchy_delete_node($node) {
  _nodehierarchy_node_records_delete($node);
}


/**
 * Can a node be a child.
 */
function nodehierarchy_node_can_be_child($node) {
  $type = is_object($node) ? $node->type : $node;
  return count(nodehierarchy_get_allowed_parent_types($type));
}

/**
 * Can a node be a parent.
 */
function nodehierarchy_node_can_be_parent($node) {
  $type = is_object($node) ? $node->type : $node;
  return count(nodehierarchy_get_allowed_child_types($type));
}

/**
 * Determine if a given node can be a child of another given node.
 *
 * @param $parent
 *    The potentential parent node (can be null for any node).
 * @param $child
 *    The potential child node (can be null for any node).
 * @return
 *   Boolean. Whether second node can be a child of the first.
 */
function nodehierarchy_node_can_be_child_of($parent = NULL, $child = NULL) {
  return in_array($child->type, nodehierarchy_get_allowed_child_types($parent->type));
}

/**
 * Get the allowed parent types for the given child type.
 */
function nodehierarchy_get_allowed_parent_types($child_type = NULL) {
  static $alowed_types = array();

  // Static cache the results because this may be called many times for the same type on the menu overview screen.
  if (!isset($alowed_types[$child_type])) {
    $parent_types = array();
    foreach (node_type_get_types() as $type => $info) {
      $allowed_children = array_filter(variable_get('nh_allowchild_' . $type, array()));
      if ((empty($child_type) && !empty($allowed_children)) || (in_array($child_type, (array) $allowed_children, TRUE))) {
        $parent_types[] = $type;
      }
    }
    $alowed_types[$child_type] = array_unique($parent_types);
  }
  return $alowed_types[$child_type];
}

/**
 * Get the allwed parent types for the given child type.
 */
function nodehierarchy_get_allowed_child_types($parent_type) {
  $child_types = array_filter(variable_get('nh_allowchild_' . $parent_type, array()));
  return array_unique($child_types);
}

/**
 * Do the actual insertion or update. No permissions checking is done here.
 */
function _nodehierarchy_save_node(&$node) {
  if (!isset($node->nodehierarchy_parents)) {
    return;
  }
  foreach ($node->nodehierarchy_parents as $i => $item) {
    $node->nodehierarchy_parents[$i] = (object)$item;
    $node->nodehierarchy_parents[$i]->cnid = $node->nid;
    if (!empty($node->nodehierarchy_parents[$i]->remove)) {
      $node->nodehierarchy_parents[$i]->pnid = NULL;
    }
    _nodehierarchy_record_save($node->nodehierarchy_parents[$i]);
  }
}

/**
 * Get the parent selector pulldown.
 */
function _nodehierarchy_get_parent_selector($child_type, $parent, $exclude = NULL) {
  // Allow other modules to create the pulldown first.
  // Modules implementing this hook, should return the form element inside an array with a numeric index.
  // This prevents module_invoke_all from merging the outputs to make an invalid form array.
  $out = module_invoke_all('nodehierarchy_get_parent_selector', $child_type, $parent, $exclude);

  if ($out) {
    // Return the last element defined (any others are thrown away);
    return end($out);
  }

  $default_value = $parent;

  // If no other modules defined the pulldown, then define it here.
  $options = array(0 => '-- ' . t('NONE') . ' --');
  $items = _nodehierarchy_parent_options($child_type, $exclude);

  foreach ($items as $key => $item) {
    $options[$key] = _nodehierarchy_parent_option_title($item);
  }

  // Make sure the current value is enabled so items can be resaved.
  if ($default_value && isset($items[$default_value])) {
    $items[$default_value]->disabled = 0;
  }

  $out = array(
    '#type' => 'select',
    '#title' => t('Parent Node'),
    '#default_value' => $default_value,
    '#attributes' => array('class' => array('nodehierarchy-parent-selector')),
    '#options' => $options,
    '#items' => $items,
    '#theme' => 'nodehierarchy_parent_selector',
    '#element_validate' => array('nodehierarchy_parent_selector_validate'),
  );
  return $out;
}


/**
 * Get the default menu link values for a new nodehierarchy menu link.
 */
function _nodehierarchy_default_record($cnid = NULL, $npid = NULL) {
  return (object)array(
    'pnid' => $npid,
    'weight' => 0,
    'cnid' => $cnid,
  );
}


/**
 * Implements hook_content_extra_fields().
 */
function _nodehierarchy_content_extra_fields($type_name) {
  $extra = array();
  if (nodehierarchy_node_can_be_child($type_name) || nodehierarchy_node_can_be_parent($type_name)) {
    $extra['nodehierarchy'] = array(
      'label' => t('Node Hierarchy settings'),
      'description' => t('Node Hierarchy module form.'),
      'weight' => 10,
    );
  }
  return $extra;
}

/**
 * Set the default parents for a node.
 */
function _nodehierarchy_nodehierarchy_default_parents(&$node) {
  if (nodehierarchy_node_can_be_child($node) || nodehierarchy_node_can_be_parent($node)) {
    if (!isset($node->nodehierarchy_parents) || empty($node->nodehierarchy_parents)) {
      // Create a default nodeheirarchy object.
      $nid = empty($node->nid) ? null : $node->nid;
      $parent = _nodehierarchy_default_record($nid, 0);

      // Set the type default if there is one.
      if (empty($node->nid)) {
        $default = variable_get('nh_defaultparent_' . $node->type, 0);
        // Get the parent node id from passed in from the get params.
        $pnid = !empty($_GET['parent']) ? (int) $_GET['parent'] : $default;
        // Get the parent from the get string. User must have update perms for parent unless it is the default.
        if ($pnid && $parent_node = node_load($pnid)) {
          if (nodehierarchy_node_can_be_parent($parent_node) && (user_access('create child of any parent') || node_access("update", $parent_node) || $parent_node->nid == $default)) {
            $parent->pnid = $pnid;
          }
        }
      }

      $node->nodehierarchy_parents[] = $parent;
    }
  }
}


/**
 * Get the next child weight for a given pnid.
 */
function _nodehierarchy_get_parent_next_child_weight($pnid) {
  $out = db_query('SELECT MAX(cweight) FROM {nodehierarchy} WHERE pnid = :pnid', array(':pnid' => $pnid))->fetchField();
  if ($out !== NULL) {
    return $out + 1;
  }
  return 0;
}


/**
 * Save a nodehierarchy record, resetting weights if applicable
 */
function _nodehierarchy_record_save(&$item) {
  if (!empty($item->nhid)) {
    // Remove the item if it's no longer needed.
    if (empty($item->pnid)) {
      _nodehierarchy_record_delete($item->nhid);
    }
    else {
      $existing_item = _nodehierarchy_record_load($item->nhid);
      // If the parent has been changed:
      if ($existing_item->pnid !== $item->pnid) {
        $item->cweight = _nodehierarchy_get_parent_next_child_weight($item->pnid);
      }
    }
  }
  else {
    $item->cweight = _nodehierarchy_get_parent_next_child_weight($item->pnid);
  }

  if ($item->pnid) {
    _nodehierarchy_record_write($item);
  }
}

/**
 * Save a nodehierarchy record.
 */
function _nodehierarchy_record_write(&$item) {
  if (!empty($item->nhid)) {
    drupal_write_record('nodehierarchy', $item, 'nhid');
  }
  else {
    drupal_write_record('nodehierarchy', $item);
  }
}

/**
 * Save a nodehierarchy record.
 */
function _nodehierarchy_record_load($nhid) {
  $result = db_select('nodehierarchy', 'nh')
              ->fields('nh')
              ->where('nhid = :nhid', array(':nhid' => $nhid))->execute();
  return $result->fetch();
}


/**
 * Save a nodehierarchy record.
 */
function _nodehierarchy_record_delete($nhid) {
  db_delete('nodehierarchy')->condition('nhid', $nhid)->execute();
}


/**
 * Delete all link from the node to its menu items.
 */
function _nodehierarchy_node_records_delete($node) {
  $nid = $node;
  if (is_object($node)) {
    $nid = $node->nid;
  }

  db_delete('nodehierarchy')
    ->condition(db_or()->condition('pnid', $nid)->condition('cnid', $nid))
    ->execute();
}


/**
 * Delete all of the descendants of the given node.
 *
 * This is not very scalable and should probably be replaced by a version which uses batch processing.
 */
function nodehierarchy_delete_descendants($nid) {
  foreach (nodehierarchy_get_node_children($nid) as $child) {
    nodehierarchy_delete_descendants($child->cnid);
    node_delete($child->cnid);
  }
}

