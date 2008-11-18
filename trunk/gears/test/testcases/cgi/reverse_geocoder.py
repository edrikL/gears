#!/usr/bin/python2.4

""" A dummy network location provider that is used for testing reverse
geocoding. For fix requests it does not return a position. For reverse geocode
requests, it returns a fixed address.
"""

class FixRequest(object):
  """ Represents a fix request. """

  def __init__(self, request_body):
    """ Initialize FixRequest with a JSON string.

    Args:
      request_body: Request body as JSON string.
    """
    # simplejson module is imported by the server, see
    # /test/runner/testwebserver.py
    json_decoder = simplejson.JSONDecoder()
    self.request = json_decoder.decode(request_body)

  def IsValidRequest(self):
    return self.request.has_key("version") and self.request.has_key("host")

  def IsReverseGeocodeRequest(self):
    if self.request.has_key("location"):
      location = self.request["location"]
      if location.has_key("latitude") and location.has_key("longitude"):
        return True
    return False


# A successful reverse-geocode response. We must include a valid latitude and
# longitude, but Gears will return to JavaScript the values from the reverse
# geocode request, not these values.
REVERSE_GEOCODE_REQUEST_RESPONSE = """{
  "location": {
    "latitude": 9.9,
    "longitude": 9.9,
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

# An empty position response, indicates no location fixes were found.
FIX_REQUEST_RESPONSE = """{}"""


def send_response(request_handler, code, response):
  """ Helper function to construct a HTTP response.

  Args:
    request_handler: The request handler instance to issue the response
    code: The integer HTTP status code for the response
    response: The string body of the response.
  """

  request_handler.send_response(code)
  if code == 200 :
    request_handler.send_header('Content-type', 'application/json')
  request_handler.end_headers()
  request_handler.outgoing.append(response)


# We accept only POST requests with Content-Type application/json.
if self.command == 'POST':
  if self.headers.get('Content-Type') == 'application/json':
    if self.body:
      fix_request = FixRequest(self.body)

      # Hard-coded rules to return desired responses
      if not fix_request.IsValidRequest():
        send_response(self, 400, "Invalid request")
      elif fix_request.IsReverseGeocodeRequest():
        send_response(self, 200, REVERSE_GEOCODE_REQUEST_RESPONSE)
      else:
        send_response(self, 200, FIX_REQUEST_RESPONSE)
    else:
      send_response(self, 400, "Empty request")
  else:
    send_response(self, 400, "Content-Type should be application/json")
else:
  send_response(self, 405, "Request must be HTTP POST.")
