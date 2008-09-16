#!/usr/bin/python2.4

""" A dummy network location provider to be used in the Geolocation API tests.

This CGI scriptlet is designed to return the constant JSON responses triggered
by special values in the position fix request supplied as the body of an HTTP
POST request. The format of a position fix request and response is documented
at http://code.google.com/p/gears/wiki/GeolocationAPI

This script is used to simulate three test cases:
 - The network location provider returns a valid fix,
 - The network location provider can't find a position fix,
 - The request to the network location provider was malformed.

These cases are simulated by returning a populated response object, a null
response object and a 400 error code respectively. The rules to this are as
follows:

 - For a fix request with a wifi tower mac address "good_mac_address",
   a good position response is returned in well formed JSON format
   (GOOD_JSON_RESPONSE below).

 - For a fix request with a wifi tower mac address "no_location_mac_address",
   an empty position response is returned in well formed JSON format
   (NO_LOCATION_RESPONSE below).

 - For a fix request with a cell id 88 (int) an HTTP 400 error code is
   returned.
"""

class FixRequest(object):
  """ A class to decode and inspect an incoming position fix request.

  The body of a position fix request is represented as JSON string. This class
  helps decode this into a dictionary object, hence allowing easy access to
  the entries in the fix request.
  """

  def __init__(self, request_body):
    """ Initialize FixRequest with a JSON string.

    Args:
      request_body: String representation of the request body, must be well
        formed JSON string.
    """
    # simplejson module is imported by the server, see
    # /test/runner/testwebserver.py
    json_decoder = simplejson.JSONDecoder()
    self.request = json_decoder.decode(request_body)

  def __GetKeyValueAsList(self, key):
    """ Finds the key in the fix request and returns its value as a list.

    If the key is not found, or its value is not a list, None is returned.

    Args:
      key: string, key to look for in the fix request.
    """
    if not self.request.has_key(key):
      return None

    key_value = self.request[key]
    # Check that the value is actually a list
    if not isinstance(key_value, list):
      return None

    return key_value

  def __HasValueInFixRequest(self, expected_value, key_name, outer_key):
    """ Checks if a key with the given value exists in the fix request.

    Args:
      exp_value: The value expected to be found in the dictionary.
      inner_key: string, the key of whose value we are interested in.
      outer_key: string, name of the dictionary, expected to contain the
        given key.
    """
    dictionary_list = self.__GetKeyValueAsList(outer_key)
    if dictionary_list == None:
      return False

    found = False
    for dictionary in dictionary_list:
      # Check if we really have a dictionary.
      if not isinstance(dictionary, dict):
        continue

      value = dictionary.get(inner_key, None)
      if (value == expected_value):
           found = True

    return found

  def HasMacAddress(self, mac_address):
    """ Check if the given mac address exists in the fix request.

    Args:
      mac_address: string, mac address to look for, no format restrictions.
    """
    return self.__HasValueInFixRequest(mac_address,
                                       'mac_address',
                                       'wifi_towers')

  def HasCellId(self, cell_id):
    """ Check if the given cell id exists in the fix request.

    Args:
      cell_id: integer, value representing the cell id to look for.
    """
    return self.__HasValueInFixRequest(cell_id, 'cell_id', 'cell_towers')


# A valid position fix response, location data for the Google London office.
GOOD_JSON_RESPONSE = """{
  "location": {
    "latitude": 51.590722643120145,
    "longitude": -1.494140625,
    "altitude": 30,
    "horizontal_accuracy": 1200,
    "vertical_accuracy": 10,
    "address": {
      "street_number": "76",
      "street": "Buckingham Palace Road",
      "postal_code": "SW1W 9TQ",
      "city": "London",
      "county": "London",
      "region": "London",
      "country": "United Kingdom",
      "country_code": "uk"
    }
  }
}"""

# An empty position fix response, indicates no location fixes were found.
NO_LOCATION_JSON_RESPONSE = """{}"""


def send_response(request_handler, code, response):
  """ Helper function to construct a basic HTTP Response body.

  Args:
    request_handler: The request handler instance to issue the response
    code: The integer HTTP status code for the response
    response: The string body of the response.
  """

  request_handler.send_response(code)
  if code == 200 :
    request_handler.send_header('Content-type', 'application/json')
  request_handler.end_headers()
  request_handler.outgoing.append (response)


# Only respond if the request is a POST
if self.command == 'POST':
  fix_request = None
  if self.body :
    request_body = self.body.popitem()[0]
    fix_request = FixRequest(request_body)

  # Hard-coded rules to return desired responses
  if fix_request.HasMacAddress("good_mac_address"):
    send_response(self, 200, GOOD_JSON_RESPONSE)
  elif fix_request.HasMacAddress("no_location_mac_address"):
    send_response(self, 200, NO_LOCATION_JSON_RESPONSE)
  elif fix_request.HasCellId(88):
    send_response(self, 400, "Error in request")
else:
  send_response(self, 405, "Please provide a POST method.")
