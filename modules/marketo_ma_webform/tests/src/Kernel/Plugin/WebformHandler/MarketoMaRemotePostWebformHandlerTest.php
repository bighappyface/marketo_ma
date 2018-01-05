<?php

namespace Drupal\Tests\marketo_ma_webform\Kernel\Plugin\WebformHandler;

use Drupal\KernelTests\Core\Entity\EntityKernelTestBase;
use Drupal\marketo_ma_webform\Plugin\WebformHandler\MarketoMaRemotePostWebformHandler;
use Drupal\webform\WebformSubmissionInterface;

/**
 * MarketoMaRemotePostWebformHandlerTest kernel tests.
 *
 * @group marketo_ma_webform
 */
class MarketoMaRemotePostWebformHandlerTest extends EntityKernelTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = ['webform', 'marketo_ma_webform'];

  /**
   * Tests getRequestData().
   */
  public function testGetRequestData() {
    // Get constructor args and mocks.
    $configuration = [
      'settings' => [
        'marketo_ma_mapping' => [
          'my_form_element_2' => 'first_name',
        ],
        'marketo_ma_oid' => 'XXX1234567890',
      ],
    ];
    $plugin_id = 'marketo_ma_remote_post_webform';
    $plugin_definition = '';
    $logger_factory = $this->getMockLoggerChannelFactory();
    $config_factory = $this->getMockConfigFactory();
    $entity_type_manager = $this->getMockEntityTypeManager();
    $conditions_validator = $this->getMockWebformSubmissionConditionsValidator();
    $module_handler = $this->getMockModuleHandler();
    $http_client = $this->getMockHttpClient();
    $message_manager = $this->getMockWebformMessageManager();

    // Mock token manager and replace method.
    $token_manager = $this->getMockTokenManager();
    $token_manager->expects($this->any())
      ->method('replace')
      ->will($this->returnArgument(0));

    // Make getRequestData public.
    $method = new \ReflectionMethod('\Drupal\marketo_ma_webform\Plugin\WebformHandler\MarketoMaRemotePostWebformHandlerTest', 'getRequestData');
    $method->setAccessible(TRUE);

    // Instantiate handler.
    $handler = new SalesforceWebToLeadPostWebformHandler($configuration, $plugin_id, $plugin_definition, $logger_factory, $config_factory, $entity_type_manager, $conditions_validator, $module_handler, $http_client, $token_manager, $message_manager);

    // Mock webform submission.
    $webform_submission_array = [
      'my_form_element_1' => 'foo',
      'my_form_element_2' => 'bar',
    ];
    $webform_submission = $this->getMockBuilder('\Drupal\webform\WebformSubmissionInterface')
      ->getMock();
    $webform_submission->expects($this->any())
      ->method('toArray')
      ->will($this->returnValue(['data' => $webform_submission_array]));

    // Check the output.
    $expected_salesforce_data = [
      'oid' => 'XXX1234567890',
      'first_name' => 'bar',
    ];
    $actual_salesforce_data = $method->invoke($handler, '', $webform_submission);
    $this->assertEquals($expected_salesforce_data, $actual_salesforce_data);
  }

  /**
   * Tests setStateUrl().
   */
  public function testSetStateUrl() {
    // Get constructor args and mocks.
    $configuration = [
      'settings' => [
        'marketo_ma_mapping' => [
          'my_form_element_2' => 'first_name',
        ],
        'marketo_ma_oid' => 'XXX1234567890',
        'marketo_ma_url' => 'foo',
      ],
    ];
    $plugin_id = 'marketo_ma_remote_post_webform';
    $plugin_definition = '';
    $logger_factory = $this->getMockLoggerChannelFactory();
    $config_factory = $this->getMockConfigFactory();
    $entity_type_manager = $this->getMockEntityTypeManager();
    $conditions_validator = $this->getMockWebformSubmissionConditionsValidator();
    $module_handler = $this->getMockModuleHandler();
    $http_client = $this->getMockHttpClient();
    $message_manager = $this->getMockWebformMessageManager();
    $token_manager = $this->getMockTokenManager();

    // Make setStateUrl public.
    $method = new \ReflectionMethod('\Drupal\marketo_ma_webform\Plugin\WebformHandler\MarketoMaRemotePostWebformHandlerTest', 'setStateUrl');
    $method->setAccessible(TRUE);

    // Instantiate handler.
    $handler = new MarketoMaRemotePostWebformHandlerTest($configuration, $plugin_id, $plugin_definition, $logger_factory, $config_factory, $entity_type_manager, $conditions_validator, $module_handler, $http_client, $token_manager, $message_manager);
    $handler->setLabel($plugin_id);

    // Check the logic.
    $method->invoke($handler, WebformSubmissionInterface::STATE_DRAFT);
    $this->assertArrayNotHasKey(WebformSubmissionInterface::STATE_DRAFT . '_url', $handler->getConfiguration()['settings']);
    $this->assertArrayNotHasKey(WebformSubmissionInterface::STATE_COMPLETED . '_url', $handler->getConfiguration()['settings']);
    $method->invoke($handler, WebformSubmissionInterface::STATE_COMPLETED);
    $this->assertArrayHasKey(WebformSubmissionInterface::STATE_COMPLETED . '_url', $handler->getConfiguration()['settings']);
  }

  /**
   * Get a mock logger channel factory.
   */
  protected function getMockLoggerChannelFactory() {
    return $this->getMockBuilder('\Drupal\Core\Logger\LoggerChannelFactoryInterface')->getMock();
  }

  /**
   * Get a mock config factory.
   */
  protected function getMockConfigFactory() {
    return $this->getMockBuilder('\Drupal\Core\Config\ConfigFactoryInterface')->getMock();
  }

  /**
   * Get a mock entity type manager.
   */
  protected function getMockEntityTypeManager() {
    return $this->getMockBuilder('\Drupal\Core\Entity\EntityTypeManagerInterface')->getMock();
  }

  /**
   * Get a mock webform submission conditions validator.
   */
  protected function getMockWebformSubmissionConditionsValidator() {
    return $this->getMockBuilder('\Drupal\webform\WebformSubmissionConditionsValidatorInterface')->getMock();
  }

  /**
   * Get a mock module handler.
   */
  protected function getMockModuleHandler() {
    return $this->getMockBuilder('\Drupal\Core\Extension\ModuleHandlerInterface')->getMock();
  }

  /**
   * Get a mock HTTP client.
   */
  protected function getMockHttpClient() {
    return $this->getMockBuilder('\GuzzleHttp\ClientInterface')->getMock();
  }

  /**
   * Get a mock token manager.
   */
  protected function getMockTokenManager() {
    return $this->getMockBuilder('\Drupal\webform\WebformTokenManagerInterface')->getMock();
  }

  /**
   * Get a mock webform message manager.
   */
  protected function getMockWebformMessageManager() {
    return $this->getMockBuilder('\Drupal\webform\WebformMessageManagerInterface')->getMock();
  }

}
