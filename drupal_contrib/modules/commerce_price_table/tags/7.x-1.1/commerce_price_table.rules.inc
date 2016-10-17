<?php

/**
 * Implements hook_rules_action_info().
 */
function commerce_price_table_rules_action_info() {
  $actions = array();

  $actions['commerce_price_table_set_price'] = array(
    'label' => t('Replace the price for a price table'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
      ),
      'quantity' => array(
        'label' => t('Quantity'),
        'type' => 'decimal',
      ),
    ),
    'group' => t('Commerce price table'),
  );

  return $actions;
}

function commerce_price_table_set_price($line_item, $quantity) {
  // If the line item contains a product, we set the price according to the
  // quantity.
  if (commerce_line_items_quantity(array($line_item), commerce_product_line_item_types())) {
    $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
    $product = commerce_product_load($line_item_wrapper->commerce_product->product_id->value());
    $item = commerce_price_table_get_amount_qty($product, $quantity);
    if (!empty($item)) {
      // Empty the price components to recalculate them.
      $line_item->commerce_unit_price[LANGUAGE_NONE][0]['data']['components'] = array();
      $price = array(
        'amount' => $item['amount'],
        'currency_code' => $item['currency_code'],
        'data' => array(),
      );

      // Alter the base price to the current one.
      $line_item_wrapper->commerce_unit_price->data = commerce_price_component_add(
              $line_item_wrapper->commerce_unit_price->value(),
              'base_price',
              $price,
              TRUE
      );

      $line_item_wrapper->commerce_unit_price->amount = $item['amount'];
    }
  }
}
