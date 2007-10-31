import unittest
from runner import TestRunner
import pmock

class TestRunnerTest(unittest.TestCase):
    
  def setUp(self):
    self.test_url = TestRunner.TEST_URL
    self.test_webserver = pmock.Mock()
    self.browser_launcher = pmock.Mock()
    self.browser_launcher.stubs().type().will(pmock.return_value("launcher"))
    self.browser_launcher.stubs().kill()


  def tearDown(self):
    self.test_webserver.verify()
    self.browser_launcher.verify()
    
    
  def testRunTestsLaunchesTestForEachBrowser(self):
    browser_launchers = [pmock.Mock(), pmock.Mock()]
    self.test_webserver.expects(pmock.once()).startServing()
    expected_results = {}
    launcher_id = 0
    for browser_launcher in browser_launchers:
      browser_launcher.stubs().type() \
                              .will(pmock.return_value(str(launcher_id)))
      expected_results[browser_launcher.type()] = "TIMED-OUT"
      launcher_id += 1

    for browser_launcher in browser_launchers:
      self.test_webserver.expects(pmock.once()) \
                                  .startTest(pmock.eq(TestRunner.TIMEOUT)) \
        .will(pmock.return_value(expected_results[browser_launcher.type()]))
      browser_launcher.expects(pmock.once()) \
                                  .launch(pmock.eq(TestRunner.TEST_URL))
      self.test_webserver.expects(pmock.once()).testResults() \
        .will(pmock.return_value(expected_results[browser_launcher.type()]))
      browser_launcher.expects(pmock.once()).kill()
      
    self.test_webserver.expects(pmock.once()).shutdown()
      
    aggregated_results = TestRunner(browser_launchers, 
                                    self.test_webserver).runTests()
    self.assertEqual(expected_results, aggregated_results)
    
    for browser_launcher in browser_launchers:
      browser_launcher.verify()
  
  
  def test_run_tests_requires_at_least_one_browser_laucher(self):
    self.assertRaises(ValueError, TestRunner, [], self.test_webserver)
    
    
  def test_run_tests_webserver_lanched_before_browser_invoked(self):
    self.test_webserver.expects(pmock.once()).startServing()
    self.test_webserver.expects(pmock.once()) \
                                .startTest(pmock.eq(TestRunner.TIMEOUT))
      
    self.browser_launcher.expects(pmock.once()) \
      .launch(pmock.eq(TestRunner.TEST_URL)) \
      .after("startTest", self.test_webserver)
    
    self.test_webserver.expects(pmock.once()) \
      .testResults() \
      .after("launch", self.browser_launcher)
      
    self.test_webserver.expects(pmock.once()).shutdown()
    
    TestRunner([self.browser_launcher], self.test_webserver).runTests()
    

  def test_browser_type_must_be_unique(self):
    browser_launcher1 = pmock.Mock()
    browser_launcher2 = pmock.Mock()
    
    duplicated_type = "same type value"
    browser_launcher1.stubs().type().will(pmock.return_value(duplicated_type))
    browser_launcher2.stubs().type().will(pmock.return_value(duplicated_type))
    
    self.assertRaises(ValueError, TestRunner, 
                      [browser_launcher1, browser_launcher2], 
                      self.test_webserver)
    
  
if __name__ == "__main__":
  unittest.main()    
  