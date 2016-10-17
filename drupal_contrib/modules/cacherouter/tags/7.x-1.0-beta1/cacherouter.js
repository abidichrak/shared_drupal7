//$Id: cacherouter.js,v 1.1.4.3 2008/12/26 02:34:50 slantview Exp $
Drupal.behaviors.cache_router_switch = function() {
  $('form#cr-cache-switch select').change(function(){
    window.location = $(this).val();
  });
}