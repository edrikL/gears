import os
import sys
import zipfile
import os.path
import shutil
import time
import stat
import socket

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
import browser_launchers
import installer
import osutils


def serverRootDir():
    return os.path.join(os.path.dirname(__file__), '../')
                    
def localIp():
  """ Returns a string with the local ip address. """
  ip = socket.gethostbyname(socket.gethostname())

  # Expect that ip is a valid looking ipv4 address.
  if len(ip.split('.')) != 4:
    raise ValueError("Host IP appears invalid: %s" % ip)
  return ip

if __name__ == '__main__':
  test_url = 'http://localhost:8001/tester/gui.html'
  suites_report = SuitesReport('TESTS-TestSuites.xml.tmpl')
  test_servers = []
  installers = []
  launchers = []
  
  if ('bin-dbg' in sys.argv[1]):
    build_type = 'dbg'
  elif ('bin-opt' in sys.argv[1]):
    build_type = 'opt'
  else:
    build_type = ''

  # Adding second webserver for cross domain tests.
  test_servers.append(TestWebserver(serverRootDir(), port=8001))
  test_servers.append(TestWebserver(serverRootDir(), port=8002))
  
  # WinCE is a special case, because it is compiled 
  # and run on different platforms.
  if len(sys.argv) > 2 and sys.argv[2] == 'wince':
    local_ip = localIp()
    launchers.append(browser_launchers.IExploreWinCeLauncher(local_ip))
    installers.append(installer.WinCeInstaller(local_ip))
    test_url = 'http://%s:8001/tester/gui.html' % local_ip
  elif osutils.osIsWin():
    launchers.append(browser_launchers.IExploreWin32Launcher('ff2profile-win'))
    launchers.append(browser_launchers.Firefox2Win32Launcher('ff2profile-win'))
    launchers.append(browser_launchers.Firefox3Win32Launcher('ff3profile-win'))
    if osutils.osIsVista():
      installers.append(installer.WinVistaInstaller())
    else:
      installers.append(installer.WinXpInstaller())
  elif osutils.osIsNix():
    if osutils.osIsMac():
      launchers.append(browser_launchers.Firefox2MacLauncher('gears-ff2'))
      launchers.append(browser_launchers.Firefox3MacLauncher('gears-ff3'))
      installers.append(installer.Firefox2MacInstaller('gears-ff2'))
      installers.append(installer.Firefox3MacInstaller('gears-ff3'))
      # TODO(ace): uncomment safari when working
      #launchers.append(browser_launchers.SafariMacLauncher())
      #installers.append(installer.SafariMacInstaller(build_type))
    else:
      launchers.append(browser_launchers.Firefox2LinuxLauncher('gears-ff2'))
      launchers.append(browser_launchers.Firefox3LinuxLauncher('gears-ff3'))
      installers.append(installer.Firefox2LinuxInstaller('gears-ff2'))
      installers.append(installer.Firefox3LinuxInstaller('gears-ff3'))
      
  gears_binaries = sys.argv[1]
  testrunner = TestRunner(launchers, test_servers, test_url)
  bootstrap = Bootstrap(gears_binaries, installers, testrunner, suites_report)
  bootstrap.invoke()
