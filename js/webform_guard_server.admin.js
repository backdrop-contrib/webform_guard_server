(function ($) {
  'use strict';

  // When a trial tier is selected on the client form, auto-populate the
  // subscription expiry date fields with today + trial_days.
  Backdrop.behaviors.webformGuardServerClientForm = {
    attach: function (context, settings) {
      var tiers = (settings.webformGuardServer && settings.webformGuardServer.tiers)
        ? settings.webformGuardServer.tiers
        : {};

      $('#edit-tier-id', context).once('wgs-tier-expiry').on('change', function () {
        var tierId = $(this).val();
        if (!tierId || !tiers[tierId] || !tiers[tierId].trial_days) {
          return;
        }

        var expiry = new Date();
        expiry.setDate(expiry.getDate() + parseInt(tiers[tierId].trial_days, 10));

        $('#edit-subscription-expires-year').val(expiry.getFullYear());
        $('#edit-subscription-expires-month').val(expiry.getMonth() + 1);
        $('#edit-subscription-expires-day').val(expiry.getDate());
      });
    }
  };

}(jQuery));
