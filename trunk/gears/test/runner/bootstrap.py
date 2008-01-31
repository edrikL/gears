import os
import sys
import zipfile
import os.path
import shutil
import time
import stat

# Workaround file permission, stat.I_WRITE not allowing delete on all systems.
DELETABLE = int('777', 8)

# Amount of time script should sleep between checking for completed builds.
POLL_INTERVAL_SECONDS = 10

class Bootstrap:
  """ Set up test environment and handles test execution. """

  # Output and temp directory.
  OUTPUT_DIR = 'output'
  INSTALLER_DIR = os.path.join(OUTPUT_DIR, 'installers')
  
  
  def __init__(self, gears_binaries, installers, testrunner, suites_report):
    """ Set test objects. 
    
    Args:
      gears_binaries: directory where the binaries will be published. 
      installers: list of Installer objects
      testrunner: TestRunner object
      suites_report: SuitesReport object that outputs test suites report
    """
    self.__gears_binaries = gears_binaries
    self.__installers = installers
    self.__testrunner = testrunner
    self.__suites_report = suites_report
    
  def invoke(self):
    """ Start everything, main method. """
    self.clean()
    self.copyFilesLocally()
    self.install()
    self.startTesting()
    self.writeResultsToFile()


  def copyFilesLocally(self):
    """ Poll until directory with installers/binaries are available.

    then copies them to local directory. 
    """
    exists = os.path.exists(self.__gears_binaries)
    while not exists:
      exists = os.path.exists(self.__gears_binaries)
      time.sleep(POLL_INTERVAL_SECONDS)
    self.__createOutputDir()
    if os.path.exists(Bootstrap.INSTALLER_DIR):
      os.chmod(Bootstrap.INSTALLER_DIR, DELETABLE)
      shutil.rmtree(Bootstrap.INSTALLER_DIR)
    shutil.copytree(self.__gears_binaries, Bootstrap.INSTALLER_DIR)


  def install(self):
    for installer in self.__installers:
      installer.install()
  

  def startTesting(self):
    self.test_results = self.__testrunner.runTests()
    
  
  def writeResultsToFile(self):
    self.__createOutputDir(True)
    stream = open('output/TESTS-TestSuites.xml', 'w')
    self.__suites_report.writeReport(self.test_results, stream)
    stream.close()


  def clean(self):
    if os.path.exists(Bootstrap.OUTPUT_DIR):
      shutil.rmtree(Bootstrap.OUTPUT_DIR)


  def __createOutputDir(self, force_recreate=False):
    if os.path.exists(Bootstrap.OUTPUT_DIR):
      if force_recreate:
        shutil.rmtree(Bootstrap.OUTPUT_DIR)
    if not os.path.exists(Bootstrap.OUTPUT_DIR):
      os.mkdir(Bootstrap.OUTPUT_DIR)


from config import Config
sys.path.extend(Config.ADDITIONAL_PYTHON_LIBRARY_PATHS)
from testrunner import TestRunner
from testwebserver import TestWebserver
from suites_report import SuitesReport
from browser_launchers import IExploreWin32Launcher
from browser_launchers import FirefoxWin32Launcher
from browser_launchers import FirefoxLinuxLauncher
from browser_launchers import FirefoxMacLauncher
from installer import WinVistaInstaller
from installer import WinXpInstaller
from installer import LinuxInstaller
from installer import MacInstaller
import osutils


def server_root_dir():
    return os.path.join(os.path.dirname(__file__), '../')
                    
if __name__ == '__main__':
  profile_name = 'gears'
  suites_report = SuitesReport('TESTS-TestSuites.xml.tmpl')
  test_servers = []
  installers = []
  launchers = []
  
  # Adding second webserver for cross domain tests.
  test_servers.append(TestWebserver(server_root_dir(), port=8001))
  test_servers.append(TestWebserver(server_root_dir(), port=8002))
  
  if osutils.osIsWin():
    launchers.append(IExploreWin32Launcher())
    launchers.append(FirefoxWin32Launcher('ffprofile-win'))
    if osutils.osIsVista():
      installers.append(WinVistaInstaller('ffprofile-win'))
    else:
      installers.append(WinXpInstaller('ffprofile-win'))
  elif osutils.osIsNix():
    if osutils.osIsMac():
      launchers.append(FirefoxMacLauncher(profile_name))
      installers.append(MacInstaller(profile_name))
    else:
      launchers.append(FirefoxLinuxLauncher(profile_name))
      installers.append(LinuxInstaller(profile_name))
      
  gears_binaries = sys.argv[1]
  testrunner = TestRunner(launchers, test_servers)
  bootstrap = Bootstrap(gears_binaries, installers, testrunner, suites_report)
  bootstrap.invoke()
