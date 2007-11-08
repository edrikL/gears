import os
import subprocess
import signal

class BrowserLauncher:
  """ Handle launching and killing of a specific test browser. """
  def launch(self, url):
    """ Do launch. 
    
    Handle chrome addresses by using the -chrome parameter.
    """
    command = [self.browser_path]
    command.extend(self.args)
    if url[0:9] == 'chrome://':
      command.append('-chrome')
    command.append(url)
    print 'launching: %s' % str(command)
    self.process = subprocess.Popen(command)

  
  def kill(self):
    """ Kill the browser process for clean-up. """
    print 'killing process: %s' % str(self.process.pid)
    os.kill(self.process.pid, signal.SIGINT)


class Win32Launcher(BrowserLauncher):
  """ Launcher for Win32 platforms. """
  def launch(self, url):
    """ Do launch. 
    
    Windows doesn't use the -chrome parameter.
    """
    command = [self.browser_path]
    command.extend(self.args)
    command.append(url)
    print 'launching: %s' % str(command)
    self.process = subprocess.Popen(command)


  def kill(self):
    """ Kill browser process. """
    import win32api
    print 'killing process: %s' % str(self.process.pid)
    win32api.TerminateProcess(int(self.process._handle), -1)


class FirefoxWin32Launcher(Win32Launcher):
  """ Launcher for firefox browser on Win32 platforms. """
  def __init__(self, profile, automated=True):
    """ Set up firefox specific variables. """
    program_files = os.getenv('PROGRAMFILES')
    self.browser_path = os.path.join(program_files, 
                                     'Mozilla Firefox\\firefox.exe')
    if automated:
      self.args = ['-profile', profile]
    else:
      self.args = []


  def type(self):
    return 'FirefoxWin32'


class IExploreWin32Launcher(Win32Launcher):
  """ Launcher for iexplorer browser on Win32 platforms. """
  def __init__(self, automated=True):
    """ Set ie specific variables. """
    program_files = os.getenv('PROGRAMFILES')
    self.browser_path = os.path.join(program_files,
                                     'internet explorer\\iexplore.exe')
    self.args = []


  def kill(self):
    """ Kill all instances of iexplore.exe
    
    Must kill ie by name and not pid for ie7 compatibility.
    """
    import win32api
    import win32pdhutil
    import win32con

    pid_list = win32pdhutil.FindPerformanceAttributesByName('iexplore')
    for pid in pid_list:
      print 'killing process: %d' % pid
      handle = win32api.OpenProcess(win32con.PROCESS_TERMINATE, 0, pid)
      win32api.TerminateProcess(handle, 0)
      win32api.CloseHandle(handle)


  def type(self):
    return 'IExploreWin32'


class FirefoxMacLauncher(BrowserLauncher):
  """ Launcher for firefox on OSX. """
  def __init__(self, profile, automated=True):
    """ Set firefox vars. """
    self.browser_path = '/Applications/Firefox.app/Contents/MacOS/firefox-bin'
    if automated:
      self.args = ['-P', profile]
    else:
      self.args = []
  

  def type(self):
    return 'FirefoxMac'


class FirefoxLinuxLauncher(BrowserLauncher):
  """ Launcher for firefox on linux, extends BrowserLauncher. """
  def __init__(self, profile, automated=True):
    """ Set linux vars. """
    home = os.getenv('HOME')
    self.browser_path = 'firefox'
    if automated:
      self.args = ['-P', profile]
    else:
      self.args = []

    
  def type(self):
    return 'FirefoxLinux'
