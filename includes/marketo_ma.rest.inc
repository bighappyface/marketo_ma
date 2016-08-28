<?php

/**
 * @file
 * Class and methods for interacting with the Marketo SOAP API.
 */

/**
 * Marketo Interface
 */
interface MarketoInterface {

  /**
   * Retrieves list of defined fields.
   */
  public function getFields();

  /**
   * Retrieves lead information.
   * 
   * @param string $key
   *   Lead Key, typically email address
   * @param string $type
   *   Lead key type, auto-detection attempted if not supplied
   */
  public function getLead($key, $type);

  /**
   * Retrieves lead activity information.
   * 
   * @param string $key
   *   Lead Key, typically email address
   * @param string $type
   *   Lead key type, auto-detection attempted if not supplied
   */
  public function getLeadActivity($key, $type);

  /**
   * Inserts or updates a lead.
   * 
   * @param array $lead
   *   Key value pairs to be associated with a lead
   * @param string $key
   *   Lead Key, typically email address
   * @param object $cookie
   *   Marketo cookie information
   * @param array $options
   *   Array of additional options to configure lead syncing
   */
  public function syncLead($lead, $key, $cookie, $options);
}

/**
 * Marketo Client
 */
class MarketoClient implements MarketoInterface {

  // Authentication http://developers.marketo.com/documentation/rest/authentication/
  protected $client_id;
  protected $client_secret;
  protected $endpoint;
  protected $identity;
  protected $token;
  protected $isFreshToken;

  // Endpoints
  const API_VERSION = 'v1';
  const IDENTITY_API = 'oauth/token';
  const LEADS_API = 'leads.json';
  const LEADS_DESCRIBE_API = 'leads/describe.json';
  const ACTIVITIES_API = 'activities.json';
  const ACTIVITIES_TYPES_API = 'activities/types.json';
  const ACTIVITIES_PAGING_TOKEN_API = 'activities/pagingtoken.json';

  protected $handle;
  protected $http_options;
  protected $response_object;
  protected $response_info;

  /**
   * Constructor for MarketoClient class.
   * 
   * @param string $user_id
   *   A User ID (aka Access Key) defined in Marketo admin
   * @param string $encryption_key
   *   A shared secret-key (aka Encryption Key) defined in Marketo admin
   * @param string $endpoint
   *   SOAP endpoint URL
   * @param array $options
   *   Optional array of SoapClient options 
   */
  public function __construct($client_id, $client_secret, $endpoint, $identity, $token = array()) {
    // When any of our required params are empty, throw exception.
    if (empty($client_id) || empty($client_secret) || empty($endpoint) || empty($identity)) {
      throw new Exception(t('One of the fields were empty: Client Id, Client Secret, Endpoint, Identity'));
    }
    $this->client_id = $client_id;
    $this->client_secret = $client_secret;
    $this->endpoint = $endpoint . '/' . MarketoClient::API_VERSION . '/';
    $this->identity = $identity;
    $this->isFreshToken = FALSE;

    $this->http_options = array();
    $this->http_options[CURLOPT_HTTPHEADER] = array();
    $this->http_options[CURLOPT_RETURNTRANSFER] = TRUE;
    $this->http_options[CURLOPT_FOLLOWLOCATION] = FALSE;

    $this->setAccessToken($token);
  }

  /**
   * Perform a GET call to server
   * 
   * Additionaly in $response_object and $response_info are the 
   * response from server and the response info as it is returned 
   * by curl_exec() and curl_getinfo() respectively.
   * 
   * @param string $url The url to make the call to.
   * @param array $http_options Extra option to pass to curl handle.
   * @return string The response from curl if any
   */
  protected function curlGet($url, $params = array(), $http_options = array()) {
    $http_options += $this->http_options;
    if (!empty($params)) {
      $url .= '?' . http_build_query($params);
    }
    $this->handle = curl_init($url);
    if (!curl_setopt_array($this->handle, $http_options)) {
      throw new RestClientException("Error setting cURL request options");
    }
    $this->response_object = curl_exec($this->handle);
    $this->http_parse_message($this->response_object);
    curl_close($this->handle);
    return $this->response_object;
  }

  /**
   * Perform a POST call to the server
   * 
   * Additionaly in $response_object and $response_info are the 
   * response from server and the response info as it is returned 
   * by curl_exec() and curl_getinfo() respectively.
   * 
   * @param string $url The url to make the call to.
   * @param string|array The data to post. Pass an array to make a http form post.
   * @param array $http_options Extra option to pass to curl handle.
   * @return string The response from curl if any
   */
  protected function curlPost($url, $fields = array(), $http_options = array()) {
    $http_options += $this->http_options;
    $http_options[CURLOPT_POST] = TRUE;
    $http_options[CURLOPT_POSTFIELDS] = $fields;
    if (is_array($fields)) {
      $http_options[CURLOPT_HTTPHEADER][] = 'Content-Type: multipart/form-data';
    }
    $this->handle = curl_init($url);
    if (!curl_setopt_array($this->handle, $http_options)) {
      throw new RestClientException("Error setting cURL request options.");
    }
    $this->response_object = curl_exec($this->handle);
    $this->http_parse_message($this->response_object);
    curl_close($this->handle);
    return $this->response_object;
  }

  /**
   * Perform a PUT call to the server
   * 
   * Additionaly in $response_object and $response_info are the 
   * response from server and the response info as it is returned 
   * by curl_exec() and curl_getinfo() respectively.
   * 
   * @param string $url The url to make the call to.
   * @param string|array The data to post.
   * @param array $http_options Extra option to pass to curl handle.
   * @return string The response from curl if any
   */
  protected function curlPut($url, $data = '', $http_options = array()) {
    $http_options = $http_options + $this->http_options;
    $http_options[CURLOPT_CUSTOMREQUEST] = 'PUT';
    $http_options[CURLOPT_POSTFIELDS] = $data;
    $this->handle = curl_init($url);
    if (!curl_setopt_array($this->handle, $http_options)) {
      throw new RestClientException("Error setting cURL request options.");
    }
    $this->response_object = curl_exec($this->handle);
    $this->http_parse_message($this->response_object);
    curl_close($this->handle);
    return $this->response_object;
  }

  /**
   * Perform a DELETE call to server
   * 
   * Additionaly in $response_object and $response_info are the 
   * response from server and the response info as it is returned 
   * by curl_exec() and curl_getinfo() respectively.
   * 
   * @param string $url The url to make the call to.
   * @param array $http_options Extra option to pass to curl handle.
   * @return string The response from curl if any
   */
  protected function curlDelete($url, $http_options = array()) {
    $http_options = $http_options + $this->http_options;
    $http_options[CURLOPT_CUSTOMREQUEST] = 'DELETE';
    $this->handle = curl_init($url);
    if (!curl_setopt_array($this->handle, $http_options)) {
      throw new RestClientException("Error setting cURL request options.");
    }
    $this->response_object = curl_exec($this->handle);
    $this->http_parse_message($this->response_object);
    curl_close($this->handle);
    return $this->response_object;
  }

  protected function http_parse_message($res) {
    if (!$res) {
      throw new HttpServerException(curl_error($this->handle), -1);
    }
    $this->response_info = curl_getinfo($this->handle);
    $code = $this->response_info['http_code'];
    if ($code == 404) {
      throw new HttpServerException404(curl_error($this->handle));
    }
    if ($code >= 400 && $code <= 600) {
      throw new HttpServerException('Server response status was: ' . $code .
      ' with response: [' . $res . ']', $code);
    }
    if (!in_array($code, range(200, 207))) {
      throw new HttpServerException('Server response status was: ' . $code .
      ' with response: [' . $res . ']', $code);
    }
  }

  public function getResponseObject() {
    return $this->response_object;
  }

  public function getResponseInfo() {
    return $this->response_info;
  }
  
  public function getAccessToken() {
    return $this->token;
  }

  protected function setAccessToken($token = array()) {
    if (!$this->isTokenValid($token)) {
      $token = $this->getNewAccessToken();
      $token->authorization = "Authorization: " . $token->token_type . " " . $token->access_token;
      $token->expires_at = time() + $token->expires_in;
      $this->isFreshToken = TRUE;
    }
    $this->http_options[CURLOPT_HTTPHEADER][] = $token->authorization;
    $this->token = $token;
  }

  protected function isTokenValid($token = array()) {
    // Token is considered invalid if it is missing a field or expires in the next 60 seconds.
    if (
      empty($token) ||
      !(array_key_exists('authorization', $token) &&
        array_key_exists('expires_at', $token)
       ) ||
      $token->expires_at < time() + 60
      ) {
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  protected function getNewAccessToken() {
    $url = $this->identity . '/' . MarketoClient::IDENTITY_API;
    $params = array(
      'grant_type' => 'client_credentials',
      'client_id' => $this->client_id,
      'client_secret' => $this->client_secret,
    );
    $response = $this->curlGet($url, $params);
    return json_decode($response);
  }

  public function getFields() {
    $url = $this->endpoint . MarketoClient::LEADS_DESCRIBE_API;
    $params = array();
    try {
      $response = $this->curlGet($url, $params);
      $fields = $this->prepareFields($response);
    }
    catch (Exception $e) {
      $fields = array();
    }
    return $fields;
  }

  public function getLead($key, $type = NULL, $fields = NULL) {
    $url = $this->endpoint . MarketoClient::LEADS_API;
    $params = array(
      'filterType' => is_null($type) ? $this->keyType($key) : strtoupper($type),
      'filterValues' => $key,
      '_method' => 'GET',
    );
    if (!is_null($fields)) {
      $params['fields'] = $fields;
    }
    $response = $this->curlPost($url, $params);
    return json_decode($response);
  }

  public function getLeadActivity($key, $type) {
    return;
  }

  public function syncLead($lead, $key, $cookie, $options) {
    return;
  }

  /**
   * Prepare an array of fields and data type.
   *
   * @param $result
   * @return array|bool
   */
  private function prepareFields($field_json) {
    $result = json_decode($field_json);
    $fields = array();
    if ($result->success && !empty($result->result)) {
      foreach ($result->result as $field) {
        $fields[$field->rest->name] = array(
          'dataType' => $field->dataType,
          'displayName' => $field->displayName,
          'id' => $field->id,
          'length' => $field->length,
          'name' => $field->rest->name,
          'readOnly' => $field->rest->readOnly,
          'soapName' => $field->soap->name,
          'soapReadOnly' => $field->soap->readOnly,
        );
      }
    }
    ksort($fields);
    return $fields;
  }
  
  /**
   * Determines lead key type for a given key.
   * 
   * @param string $key
   *   The key to examine
   * 
   * @return string
   *   Lead key type
   */
  protected function keyType($key) {
    if (filter_var($key, FILTER_VALIDATE_EMAIL)) {
      $type = 'EMAIL';
    }
    elseif (is_int($key) || (is_string($key) && ctype_digit($key))) {
      $type = 'ID';
    }
    elseif (filter_var($key, FILTER_VALIDATE_REGEXP, array('options' => array('regexp' => '/^id:.*&token:.*/')))) {
      $type = 'COOKIE';
    }
    else {
      $type = 'UNKNOWN';
    }

    return $type;
  }
}

class MarketoError {

  const ERR_REQ_ENTITY_TOO_LARGE = 413;
  const ERR_REQ_URI_TOO_LONG = 414;
  const ERR_BAD_GATEWAY = 502;
  const ERR_ACCESS_TOKEN_EMPTY = 600;
  const ERR_ACCESS_TOKEN_INVALID = 601;
  const ERR_ACCESS_TOKEN_EXPIRED = 602;
  const ERR_ACCESS_DENIED = 603;
  const ERR_REQ_TIMEOUT = 604;
  const ERR_HTTP_METHOD_UNSUPPORTED = 605;
  const ERR_MAX_RATE_LIMIT_EXCEEDED = 606;
  const ERR_DAILY_QUOTA_REACHED = 607;
  const ERR_API_TEMP_UNAVAILABLE = 608;
  const ERR_JSON_INVALID = 609;
  const ERR_REQ_RESOURCE_NOT_FOUND = 610;
  const ERR_SYSTEM_ERROR = 611;
  const ERR_CONTENT_TYPE_INVALID = 612;
  const ERR_MULTIPART_REQ_INVALID = 613;
  const ERR_CONCURRENT_ACCESS_LIMIT = 615;
  const ERR_FIELD_EMPTY = 701;
  const ERR_RECORDS_NOT_FOUND = 702;
  const ERR_FEATURE_NOT_ENABLED = 703;
  const ERR_DATE_FORMAT_INVALID = 704;
  const ERR_BUSINESS_RULE_VIOLATION = 709;
  const ERR_PARENT_FOLDER_NOT_FOUND = 710;
  const ERR_FOLDER_TYPE_INCOMPATIBLE = 711;
  const ERR_PARAM_TYPE_MISMATCH = 1001;
  const ERR_PARAM_MISSING_VALUE = 1002;
  const ERR_DATA_INVALID = 1003;
  const ERR_LEAD_NOT_FOUND = 1004;
  const ERR_LEAD_ALREADY_EXISTS = 1005;
  const ERR_FIELD_NOT_FOUND = 1006;
  const ERR_MULTIPLE_LEAD_MATCH = 1007;
  const ERR_PARTITION_ACCESS_DENIED = 1008;
  const ERR_PARTITION_NAME_UNSPECIFIED = 1009;
  const ERR_PARTITION_UPDATE_NOT_ALLOWED = 1010;
  const ERR_FIELD_NOT_SUPPORTED = 1011;
  const ERR_COOKIE_VAL_INVALID = 1012;
  const ERR_OBJECT_NOT_FOUND = 1013;
  const ERR_OBJECT_CREATE_FAILED = 1014;
  const ERR_LEAD_NOT_IN_LIST = 1015;
  const ERR_TOO_MANY_IMPORTS = 1016;
  const ERR_OBJECT_ALREADY_EXISTS = 1017;
  const ERR_CRM_ENABLED = 1018;
  const ERR_IMPORT_IN_PROGRESS = 1019;
  const ERR_TOO_MANY_CLONE_OP = 1020;
  const ERR_COMPANY_UPDATE_NOT_ALLOWED = 1021;
  const ERR_OBJECT_IN_USE = 1022;

}

class HttpServerException extends Exception {
}
class HttpServerException404 extends Exception {
  function __construct($message = 'Not Found') {
    parent::__construct($message, 404);
  }
}
class RestClientException extends Exception {
}