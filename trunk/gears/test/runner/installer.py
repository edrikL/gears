import os
import shutil
import stat
import zipfile
import time
import subprocess

if os.name == 'nt':
  import win32file

# Workaround permission, stat.I_WRITE not allowing delete on some systems
DELETABLE = int('777', 8)

class Installer:
  """ Handle extension installation and browser profile adjustments. """
  GUID = '{000a9d1c-beef-4f90-9363-039d445309b8}'
  BUILDS = 'output/installers'

  def prepareProfiles(self):
    """ Unzip profiles. """
    profile_dir = 'profiles'
    profiles = os.listdir(profile_dir)
    for profile in profiles:
      profile_path = os.path.join(profile_dir, profile)
      target_path = profile[:profile.find('.')]
      if os.path.exists(target_path):
        os.chmod(target_path, DELETABLE)
        shutil.rmtree(target_path, onerror=self.handleRmError)
      profile_zip = open(profile_path, 'rb')
      self.unzip(profile_zip, target_path)
      profile_zip.close()
      

  def installExtension(self, build):
    """ Locate the desired ff profile, overwrite it, and unzip build to it. 
    
    Args:
      build: local filename for the build
    """
    if os.path.exists('xpi'):
      shutil.rmtree('xpi', onerror=self.handleRmError)
    xpi = open(build, 'rb')
    self.unzip(xpi, 'xpi')
    xpi.close()
    self.__copyProfileAndInstall('xpi')


  def __copyProfileAndInstall(self, extension):
    """ Profile and extension placement for mac/linux.
    
    Args:
      extension: path to folder containing extension to install
    """
    profile_folder = self.findProfileFolderIn(self.ff2profile_path)

    if profile_folder:
      print 'copying over profile...'
    else:
      print 'failed to find profile folder'
      return

    shutil.rmtree(profile_folder, onerror=self.handleRmError)
    self.copyAndChmod(self.ff2profile, profile_folder)
    ext = os.path.join(profile_folder, 'extensions')
    if not os.path.exists(ext):
      os.mkdir(ext)
    gears = os.path.join(ext, Installer.GUID)
    self.copyAndChmod(extension, gears)


  def findProfileFolderIn(self, path):
    """ Find and return the right profile folder in directory at path.
    
    Args:
      path: path of the directory to search in
    
    Returns:
      String name of the folder for the desired profile, or False
    """
    dir = os.listdir(path)
    for folder in dir:
      if folder.find('.' + self.profile) > 0:
        profile_folder = os.path.join(path, folder)
        return profile_folder
    return False


  def findBuildPath(self, type, directory):
    """ Find the path to the build of the given type.
  
    Args:
      type: os string eg 'win32' 'osx' or 'linux'
    
    Returns:
      String path to correct build for the type, else throw
    """
    dir_list = os.listdir(directory)
    for item in dir_list:
      if item.find(type) > -1:
        build = os.path.join(directory, item)
        build = build.replace('/', os.sep).replace('\\', os.sep)
        return build
    raise "Can't locate build of type: %s" % type


  def saveInstalledBuild(self):
    """ Copies given build to the "current build" location. """
    if os.path.exists(self.current_build):
      shutil.rmtree(self.current_build, onerror=self.handleRmError)
    shutil.copytree(Installer.BUILDS, self.current_build)


  def copyAndChmod(self, src, targ):
    shutil.copytree(src, targ)
    os.chmod(targ, DELETABLE)
  

  def handleRmError(self, func, path, exc_info):
    """ Handle errors removing files with long names on nt systems.

    Args:
      func: function call that caused exception
      path: path to function call argument
      exc_info: string info about the exception
    """
    # On nt, try using win32file to delete if os.remove fails
    if os.name == 'nt':
      # DeleteFileW can only operate on an absolute path
      if not os.path.isabs(path):
        path = os.path.join(os.getcwd(), path)
      unicode_path = '\\\\?\\' + path
      # Throws an exception on error
      win32file.DeleteFileW(unicode_path)
    else:
      raise StandardError(exc_info)
    

  def unzip(self, file, target):
    """ Unzip file to target dir.

    Args: 
      file: file pointer to archive
      target: path for folder to unzip to

    Returns:
      True if successful, else False
    """
    if not target:
      print 'invalid path'
      return False

    os.mkdir(target)
    try:
      zf = zipfile.ZipFile(file)
    except zipfile.BadZipfile:
      print 'invalid zip archive'
      return False

    archive = zf.namelist()
    for thing in archive:
      fullpath = thing.split('/')
      path = target
      for p in fullpath[:-1]:
        try:
          os.mkdir(os.path.join(path, p))
        except OSError:
          pass
        path = os.path.join(path, p)
      if thing[-1:] != '/':
        bytes = zf.read(thing)
        nf = open(os.path.join(target, thing), 'wb')
        nf.write(bytes)
        nf.close()
    return True


class Win32Installer(Installer):
  """ Installer for Win32 machines, extends Installer. """
  def install(self):
    """ Do installation.  """
    # First, uninstall current installed build, if any exists
    if os.path.exists(self.current_build):
      build_path = self.buildPath(self.current_build)
      print 'Uninstalling build %s' % build_path
      c = ['msiexec', '/passive', '/uninstall', build_path]
      p = subprocess.Popen(c)
      p.wait()

    # Now install new build
    build_path = self.buildPath(Installer.BUILDS)
    print 'Installing build %s' % build_path
    c = ['msiexec', '/passive', '/i', build_path]
    p = subprocess.Popen(c)
    p.wait()
    self.__copyProfile()

    # Save new build as current installed build
    self.saveInstalledBuild()
  

  def __copyProfile(self):
    """ Copy IE profile to correct location. """
    google_path = os.path.join(self.appdata_path, 'Google')
    ieprofile_path = os.path.join(google_path, 
                                  'Google Gears for Internet Explorer')
    if not os.path.exists(google_path):
      os.mkdir(google_path)
    if os.path.exists(ieprofile_path):
      os.chmod(ieprofile_path, DELETABLE)
      shutil.rmtree(ieprofile_path, onerror=self.handleRmError)
    self.copyAndChmod(self.ieprofile, ieprofile_path)


class WinXpInstaller(Win32Installer):
  """ Installer for WinXP, extends Win32Installer. """
  def __init__(self):
    """ Set up xp specific variables. """
    self.prepareProfiles()
    home = os.getenv('USERPROFILE')
    self.current_build = os.path.join(home, 'current_gears_build')
    self.appdata_path = os.path.join(home, 'Local Settings\\Application Data')
    self.ieprofile = 'ieprofile'


  def buildPath(self, directory):
    return self.findBuildPath('msi', directory)


  def type(self):
    return 'WinXpInstaller'


class WinVistaInstaller(Win32Installer):
  """ Installer for Vista, extends Win32Installer. """
  def __init__(self):
    self.prepareProfiles()
    home = os.getenv('USERPROFILE')
    self.current_build = os.path.join(home, 'current_gears_build')
    self.appdata_path = os.path.join(home, 'AppData\\LocalLow')
    self.ieprofile = 'ieprofile'


  def buildPath(self, directory):
    return self.findBuildPath('msi', directory)

  
  def type(self):
    return 'WinVistaInstaller'


class WinCeInstaller(Installer):
  """ Installer for WinCE, extends Installer. """
  def __init__(self, host):
    self.ieprofile = 'ieprofile'
    self.host = host
  
  
  def buildPath(self, directory):
    return self.findBuildPath('cab', directory)

  
  def type(self):
    return 'WinCeInstaller'
  
  
  def install(self):
    """ Do installation. """
    print 'Installing and copying permissions.'
    self.installCab()
    self.copyPermissions()
  
  
  def installCab(self):
    """ Copy installer to device and install.  """
    build_path = self.buildPath(Installer.BUILDS)
    
    # Requires cecopy.exe in path.
    copy_cmd = ['cecopy.exe', build_path, 'dev:\\windows\\gears.cab']
    p = subprocess.Popen(copy_cmd)
    p.wait()

    # Requires rapistart.exe in path.  Option /noui for silent install.
    install_cmd = ['rapistart.exe', '\\windows\\wceload.exe', '/noui', 
                   '\\windows\\gears.cab']

    # TODO(ace): Find a more robust solution than waiting a set timeout
    # for installation to finish.
    gears_installer_timeout = 45 #seconds
    subprocess.Popen(install_cmd)
    time.sleep(gears_installer_timeout)

    # Requires pkill.exe in path.
    kill_cmd = ['pkill.exe', 'iexplore.exe']
    p = subprocess.Popen(kill_cmd)
    p.wait()
  

  def copyPermissions(self):
    """ Modify permissions file to include host address and copy to device. """
    perm_path = os.path.join(self.ieprofile, 'permissions.db')
    perm_path.replace('/', '\\')

    # Requires sqlite3.exe in path to modify permissions db file.
    modify_cmd = ['sqlite3', perm_path]
    p = subprocess.Popen(modify_cmd, stdin=subprocess.PIPE)

    # Sql command to add gears permissions to the address of the host server.
    sql_cmd = 'INSERT INTO "Access" VALUES(\'http://%s:8001\',1);' % self.host
    p.stdin.write(sql_cmd)
    p.stdin.close()
    p.wait()

    # Requires cecopy.exe in path to copy permissions to device.
    copy_cmd = ['cecopy.exe', self.ieprofile, 
                'dev:\\application data\\google\\'
                'Google Gears for Internet Explorer']
    p = subprocess.Popen(copy_cmd)
    p.wait()


class MacInstaller(Installer):
  """ Installer for Mac, extends Installer. """
  def __init__(self, profile_name):
    """ Set mac specific variables. """
    self.profile = profile_name
    self.prepareProfiles()
    home = os.getenv('HOME')
    ff2profile = 'Library/Application Support/Firefox/Profiles'
    ffcache = 'Library/Caches/Firefox/Profiles'
    self.current_build = os.path.join(home, 'current_gears_build')
    self.firefox = '/Applications/Firefox.app/Contents/MacOS/firefox-bin'
    self.ff2profile_path = os.path.join(home, ff2profile)
    self.ffcache_path = os.path.join(home, ffcache)
    self.ff2profile = 'ff2profile-mac'
    self.profile_arg = '-CreateProfile %s' % self.profile
  
  
  def buildPath(self, directory):
    return self.findBuildPath('xpi', directory)

  
  def type(self):
    return 'MacInstaller'


  def install(self):
    """ Do installation. """
    print 'Creating test profile and inserting extension'
    build_path = self.buildPath(Installer.BUILDS)
    os.system('%s %s' % (self.firefox, self.profile_arg))
    self.installExtension(build_path)
    self.__copyProfileCacheMac()
    self.saveInstalledBuild()


  def __copyProfileCacheMac(self):
    """ Copy cache portion of profile on mac. """
    profile_folder = self.findProfileFolderIn(self.ffcache_path)

    if profile_folder:
      print 'copying profile cache...'
    else:
      print 'failed to find profile folder'
      return

    # Empty cache and replace only with gears folder
    gears_folder = os.path.join(profile_folder, 'Google Gears for Firefox')
    ff2profile_cache = 'ff2profile-mac/Google Gears for Firefox'
    shutil.rmtree(profile_folder, onerror=self.handleRmError)
    os.mkdir(profile_folder)
    self.copyAndChmod(ff2profile_cache, gears_folder)


class LinuxInstaller(Installer):
  """ Installer for linux, extends Installer. """
  def __init__(self, profile_name):
    """ Set linux specific variables. """
    self.profile = profile_name
    self.prepareProfiles()
    home = os.getenv('HOME')
    self.current_build = os.path.join(home, 'current_gears_build')
    self.ff2profile_path = os.path.join(home, '.mozilla/firefox')
    self.firefox = 'firefox'
    self.ff2profile = 'ff2profile-linux'
    self.profile_arg = '-CreateProfile %s' % self.profile


  def buildPath(self, directory):
    return self.findBuildPath('xpi', directory)


  def type(self):
    return 'LinuxInstaller'


  def install(self):
    """ Do installation. """
    print 'Creating test profile and inserting extension'
    build_path = self.buildPath(Installer.BUILDS)
    os.system('%s %s' % (self.firefox, self.profile_arg))
    self.installExtension(build_path)
    self.saveInstalledBuild()
