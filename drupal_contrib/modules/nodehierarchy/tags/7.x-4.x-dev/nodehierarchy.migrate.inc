<?php

/**
 * @file
 * Support for the Pathauto module.
 */

/**
 * Field handler.
 */
class MigrateNodeHierarchyHandler extends MigrateDestinationHandler {
  public function __construct() {
    $this->registerTypes(array('node'));
  }

  /**
   * Make the destination field visible.
   */
  public function fields() {
    if (module_exists('nodehierarchy')) {
      return array(
        'nodehierarchy:pnid' =>
          t('NodeHierarchy Parent: The node id of the parent node.'),
        'nodehierarchy:customized' =>
          t('NodeHierarchy Customized: Whether the menu title is customized.'),
        'nodehierarchy:enabled' =>
          t('NodeHierarchy Menu Enabled: Whether the menu is enabled.'),
        'nodehierarchy:description' =>
          t('NodeHierarchy Menu Description: The menu description.'),
        'nodehierarchy:link_title' =>
          t('NodeHierarchy Menu Title'),
        'nodehierarchy:expanded' =>
          t('Is the NodeHierarchy menu expanded.'),
        'nodehierarchy:weight' =>
          t('NodeHierarchy Menu Description: The menu description.'),
        'nodehierarchy:view' =>
          t('NodeHierarchy View: The node embedded children view for this node.'),
      );
    }
    else {
      return array();
    }
  }

  public function prepare($entity, stdClass $row) {
    if (!empty($entity->nodehierarchy['arguments']['pnid']) || !empty($entity->nodehierarchy['arguments']['enabled'])) {
      // Load the existing menu items and defaults if any.
      nodehierarchy_node_prepare($entity);

      // Remove all but the primary parent.
      foreach ($entity->nodehierarchy_parents as $i => $parent) {
        // Keep the first one in case it has a menu which needs to be moved.
        if ($i !== 0) {
          $entity->nodehierarchy_parents[$i]->remove = TRUE;
        }
      }

      $entity->nodehierarchy_parents[0]->cnid = $entity->nid;
      $entity->nodehierarchy_parents[0]->pnid = $entity->nodehierarchy['arguments']['pnid'];
      $entity->nodehierarchy_parents[0]->cweight = $entity->nodehierarchy['arguments']['weight'];
  
      if (is_array($entity->nodehierarchy['arguments']['pnid'])) {
        $entity->nodehierarchy_parents[0]->pnid = array_shift($entity->nodehierarchy['arguments']['pnid']);
      }
      if (is_array($entity->nodehierarchy['arguments']['weight'])) {
        $entity->nodehierarchy_parents[0]->cweight = array_shift($entity->nodehierarchy['arguments']['weight']);
      }

      if (!empty($entity->nodehierarchy['arguments']['enabled'])) {
        foreach ($entity->nodehierarchy['arguments'] as $key => $val) {
          $entity->nodehierarchy_parents[0]->menu_link[$key] = $val;
        }
      }
    }
    // Add the embed view.
    if ($entity->nodehierarchy['arguments']['view']) {
      $entity->nh_children_view = $entity->nodehierarchy['arguments']['view'];
    }

    // Add any non-primary parents
    if (is_array($entity->nodehierarchy['arguments']['pnid'])) {
      foreach ($entity->nodehierarchy['arguments']['pnid'] as $i => $pnid) {
        $parent = _nodehierarchy_default_record($entity->nid, $entity->nodehierarchy['arguments']['pnid']);
        if (isset($entity->nodehierarchy['arguments']['pnid'][$i])) {
          $parent->cweight = $entity->nodehierarchy['arguments']['pnid'][$i];
        }
        $entity->nodehierarchy_parents[] = $parent;
      }
    }

    unset($entity->nodehierarchy);
  }
}
