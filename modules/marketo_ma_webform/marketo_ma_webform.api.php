<?php

/**
 * @file
 * Hooks related to Webform module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Alter the data to be posted to Marketo.
 *
 * @param array $data
 *   The array of data to be posted, keyed on the machine-readable element name.
 */
function hook_marketo_ma_webform_posted_data_alter(array &$data, Drupal\webform\Entity\Webform $webform, Drupal\webform\WebformSubmissionInterface $webform_submission) {

}
