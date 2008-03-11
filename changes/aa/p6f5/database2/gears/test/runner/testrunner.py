import sys

class TestRunner:
  """ Run browser tests.
  
  This class manages execution in-browser JavaScript tests. Uses webserver
  and list of browser launcher objects to run the test on 
  all specified browsers.

  Args:
    browser_launchers: list of BrowserLauncher types
    test_server: instance of testwebserver
  """
  
  TEST_URL = "http://localhost:8001/tester/gui.html"
  TIMEOUT = 90
    
  def __init__(self, browser_launchers, web_servers):
    if not browser_launchers or len(browser_launchers) < 1:
      raise ValueError("Please provide browser launchers")
    self.__verifyBrowserLauncherTypesUnique(browser_launchers)
    self.browser_lauchers = browser_launchers
    self.web_servers = web_servers


  def runTests(self, automated=True):
    """ Launch tests on the test webserver for each launcher.

    Returns:
      results object keyed by browser.
    """
    test_results = {}

    # Only one instance of TestWebserver must call startServing.
    # Expect postback results from the first server on the list.
    test_server = self.web_servers[0]
    test_server.startServing()

    try:
      for browser_launcher in self.browser_lauchers:
        test_server.startTest(TestRunner.TIMEOUT)
        try:
          browser_launcher.launch(TestRunner.TEST_URL)
        except:
          self.__handleBrowserTestCompletion(browser_launcher, 
                                             test_results, automated)
          print "Error launching browser ", sys.exc_info()[0]
        else: 
          # There is not try/catch/finally available to us so
          # we will go with code duplication.
          self.__handleBrowserTestCompletion(browser_launcher, 
                                             test_results, automated)
    finally:
      # Shutdown each instance of TestWebserver after testing is complete
      # to unbind sockets.
      for server in self.web_servers:
        server.shutdown()
      return test_results


  def __handleBrowserTestCompletion(self, browser_launcher, test_results, 
                                    automated):
    """ Extract results and kill the browser. """
    test_server = self.web_servers[0]
    test_results[browser_launcher.type()] = test_server.testResults()
    if automated:
      try:
        browser_launcher.kill()
      except:
        print "Error killing browser ", sys.exc_info()[0]
          

  def __verifyBrowserLauncherTypesUnique(self, browser_launchers):
    """ Check that the given launchers represent unique browser type.

    Args:
      browser_launchers: list of BrowserLauncher objects.
    """
    browser_launchers_by_name = {}
    for browser_launcher in browser_launchers:
      browser_type = browser_launcher.type()
      if not browser_launchers_by_name.has_key(browser_type):
        browser_launchers_by_name[browser_type] = []
      browser_launchers_by_name[browser_type].append(browser_launcher)
        
    for launchers in browser_launchers_by_name.values():
      if len(launchers) > 1:
        raise ValueError("Browser launchers all must have unique type values")


if __name__ == '__main__':
  """ If run as main, launch tests on current system. """
  import sys
  import os
  from testwebserver import TestWebserver
  import browser_launchers as launcher
  import osutils

  from config import Config
  sys.path.extend(Config.ADDITIONAL_PYTHON_LIBRARY_PATHS)

  def server_root_dir():
      return os.path.join(os.path.dirname(__file__), '../')

  web_servers = []
  web_servers.append(TestWebserver(server_root_dir(), port=8001))
  web_servers.append(TestWebserver(server_root_dir(), port=8002))

  installers = []

  if osutils.osIsWin():
    launchers = []
    launchers.append(launcher.IExploreWin32Launcher(automated=False))
    launchers.append(launcher.FirefoxWin32Launcher('ffprofile-win', 
                                                   automated=False))
  
  elif osutils.osIsNix():
    if osutils.osIsMac():
      launchers = [launcher.FirefoxMacLauncher('gears', automated=False)]

    else: #is linux
      launchers = [launcher.FirefoxLinuxLauncher('gears', automated=False)]

  testrunner = TestRunner(launchers, web_servers)
  testrunner.runTests(automated=False)
