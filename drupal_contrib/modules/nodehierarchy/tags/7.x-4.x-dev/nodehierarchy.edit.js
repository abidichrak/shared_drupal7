(function ($) {

Drupal.behaviors.nodehierarchyFieldsetSummaries = {
  attach: function (context) {
    $('fieldset.nodehierarchy-form', context).drupalSetSummary(function (context) {
      if ($('.nodehierarchy-parent-selector', context).length) {
        var parent = $('.nodehierarchy-parent-selector', context).val();
        if (parent == '0') {
          return Drupal.t('No parent');
        }
        else {
          var title = $('.nodehierarchy-parent-selector').find('[value=' + parent + ']').text().replace(/\-+/, '');
          return Drupal.t('Parent: @title', {'@title': title});
        }
      }
      else if ($('.nodehierarchy-parent-autocomplete', context).length) {
        var parent = $('.nodehierarchy-parent-autocomplete', context).val();
        if (!parent) {
          return Drupal.t('No parent');
        }
        else {
          var title = parent.replace(/\[nid\:\d+\]/, '');
          return Drupal.t('Parent: @title', {'@title': title});
        }
      }
    });
  }
};

})(jQuery);
