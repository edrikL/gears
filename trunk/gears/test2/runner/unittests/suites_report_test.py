import unittest
import StringIO
import runner
from runner import SuitesReport

class SuitesReportTest(unittest.TestCase):


  SINGLE_FAILURE = \
    {
      'browser1' : 
      {
        'browser_info':'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; ' \
                                        'rv:1.8.1.7) Gecko/20070914',
        'gears_info': '0.2.4.0;official;opt;firefox',
        'url': 'http://localhost:8001/runner/gui.html',
        'results' : 
          [
           {
            'filename': '/testcases/factory_tests.js',
            'suitename': 'Factory',
            'results': 
              {
                  'testCheckVersionProperty' : {'status': 'passed'},
                  'testSomeFailure': 
                    {
                      'message': 'Expected: 1 (number), actual: 2 (number)', 
                      'status': 'failed'
                    } 
              }
            }
           ]
      }
    }
    
    
  MULTIPLE_BROWSERS = \
    {
      'browser1' : 
      {
        'browser_info':'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US;' \
                                        ' rv:1.8.1.7) Gecko/20070914',
        'gears_info': '0.2.4.0;official;opt;firefox',
        'url': 'http://localhost:8001/runner/gui.html',
        'results' : [
          {
            'filename': '/filename1.js',
            'suitename': 'suite1',
            'results': {'testOne' : {'status': 'passed'},}
          }
          ]
      },
      'browser2' : 
      {
        'browser_info':'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; ' \
                                        'rv:1.8.1.7) Gecko/20070914',
        'gears_info': '0.2.4.0;official;opt;firefox',
        'url': 'http://localhost:8001/runner/gui.html',
        'results' : [
          {
            'filename': '/filename1.js',
            'suitename': 'suite1',
            'results': {'testOne' : {'status': 'passed'},}
          }
          ]
      }      
    }    


  MULTIPLE_SUITES = \
    {
      'browser1' : 
      {
        'browser_info': 'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; '
                        'rv:1.8.1.7) Gecko/20070914',
        'gears_info': '0.2.4.0;official;opt;firefox',
        'url': 'http://localhost:8001/runner/gui.html',
        'results' : [
          {
            'filename': '/filename1.js',
            'suitename': 'suite1',
            'results': 
              {
                  'testOne' : {'status': 'passed'}
              }
          },
          {
            'filename': '/filename1.js',
            'suitename': 'suite2',
            'results': 
              {
                  'testTwo' : {'status': 'failed'},
                  'testThree' : {'status': 'passed'},
              }
          }
          ]
      }
    }
    
    
  TIMED_OUT = {'FirefoxLinux' : 'TIMED-OUT'}
  
  SAMPLE_OUTPUT = \
    {'browser1': 
      {'url': 'http://localhost:8001/runner/gui.html', 
       'suites': 
        {'Factory': 
         [
          {'results': 
           {
            'testCheckVersionProperty': {'status': 'passed'}, 
            'testSomeFailure': {'status': 'failed', 'message': 
                                'Expected: 1 (number), actual: 2 (number)'}
          }, 
          'filename': '/testcases/factory_tests.js'
          }
         ]
         }, 
        'browser_info': 
            'Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; '
            'rv:1.8.1.7) Gecko/20070914', 
        'gears_info': '0.2.4.0;official;opt;firefox'
      }
    }


  def setUp(self):
    self.suites_report = SuitesReport('TESTS-TestSuites.xml.tmpl')
    
 
  def testGroupsByBrowser(self):
    result = self.suites_report.groupResultsBySuites(
        SuitesReportTest.MULTIPLE_BROWSERS)
    expected = ['browser1', 'browser2']
    expected.sort()
    self.assertEqual( expected, result.keys())

  def testGroupsBySuites(self):
    result = self.suites_report.groupResultsBySuites(
        SuitesReportTest.MULTIPLE_SUITES)
    self.assertEqual(['browser1'], result.keys())
    actual_browser_results = result['browser1']
    expected_browser_results = SuitesReportTest.MULTIPLE_SUITES['browser1']
    for key in ['browser_info', 'gears_info', 'url']:
      self.assertEqual(
          expected_browser_results[key], actual_browser_results[key], 
          'Wrong value for key %s' %key)
    
    actual = actual_browser_results['suites'].keys()
    self.assertEqual(['suite2', 'suite1'], actual)

  
  def testSuiteResults(self):
    result = self.suites_report.groupResultsBySuites(
        SuitesReportTest.SINGLE_FAILURE)
    suites_level_results = result['browser1']['suites']
    self.assertEqual(['Factory'], suites_level_results.keys())
    actual_results = suites_level_results['Factory']
    self.assertTrue(1, len(actual_results))
    self.assertEqual('/testcases/factory_tests.js', 
                     actual_results[0]['filename'])
    self.assertEqual(
        SuitesReportTest.SINGLE_FAILURE['browser1']['results'][0]['results'], 
        actual_results[0]['results'])

  def testSuiteResultsCompleteMatch(self):
    result = self.suites_report.groupResultsBySuites(
        SuitesReportTest.SINGLE_FAILURE)
    self.assertEqual(SuitesReportTest.SAMPLE_OUTPUT, result)
        
  def testTimedOutResults(self):
    result = self.suites_report.groupResultsBySuites(
        SuitesReportTest.TIMED_OUT)
    self.assertEqual(result['FirefoxLinux'], 'TIMED-OUT')


  def testWriteReport(self):
    # no assertions, XMLUnit would be handy
    output = StringIO.StringIO()
    self.suites_report.writeReport(SuitesReportTest.MULTIPLE_SUITES, output)
