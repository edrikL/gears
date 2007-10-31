import os
import shutil
import stat
import subprocess
import zipfile

# Workaround permission, stat.I_WRITE not allowing delete on some systems
DELETABLE = int('777', 8)

class Installer:
  """ Handle extension installation and browser profile adjustments. """
  GUID = '{000a9d1c-beef-4f90-9363-039d445309b8}'
  PROFILE = 'gears'

  def prepareProfiles(self):
    """ Unzip profiles. """
    profile_dir = 'profiles'
    profiles = os.listdir(profile_dir)
    for profile in profiles:
      profile_path = os.path.join(profile_dir, profile)
      target_path = profile[:profile.find('.')]
      if os.path.exists(target_path):
        os.chmod(target_path, DELETABLE)
        shutil.rmtree(target_path)
      profile_zip = open(profile_path, 'rb')
      self.unzip(profile_zip, target_path)
      profile_zip.close()
      

  def installExtension(self, build):
    """ Locate the desired ff profile, overwrite it, and unzip build to it. 
    
    Args:
      build: local filename for the build
    """
    if os.path.exists('xpi'):
      shutil.rmtree('xpi')
    xpi = open(build, 'rb')
    self.unzip(xpi, 'xpi')
    xpi.close()
    self.__copyProfileAndInstall('xpi')


  def __copyProfileAndInstall(self, extension):
    """ Profile and extension placement for mac/linux.
    
    Args:
      extension: path to folder containing extension to install
    """
    profile_folder = self.findProfileFolderIn(self.ffprofile_path)

    if profile_folder:
      print 'copying over profile...'
    else:
      print 'failed to find profile folder'
      return

    shutil.rmtree(profile_folder)
    self.copyAndChmod(self.ffprofile, profile_folder)
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
      if folder.find('.' + Installer.PROFILE) > 0:
        profile_folder = os.path.join(path, folder)
        return profile_folder
    return False

  def findBuildPath(self, type):
    """ Find the path to the build of the given type.
  
    Args:
      type: os string 'win32' 'osx' or 'linux'
    
    Returns:
      String path to correct build for the type, else throw
    """
    search_dir = 'output/installers'
    dir_list = os.listdir(search_dir)
    for item in dir_list:
      if item.find(type) > -1:
        build = os.path.join(search_dir, item)
        build = build.replace('/', os.sep).replace('\\', os.sep)
        return build
    raise "Can't locate build of type: %s" % type

  
  def copyAndChmod(self, src, targ):
    shutil.copytree(src, targ)
    os.chmod(targ, DELETABLE)


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
    os.system(self.buildPath())
    self.__copyProfile()

  
  def __copyProfile(self):
    """ Copy IE profile to correct location. """
    google_path = os.path.join(self.appdata_path, 'Google')
    ieprofile_path = os.path.join(google_path, 
                                  'Google Gears for Internet Explorer')
    if not os.path.exists(google_path):
      os.mkdir(google_path)
    if os.path.exists(ieprofile_path):
      os.chmod(ieprofile_path, DELETABLE)
      shutil.rmtree(ieprofile_path)
    self.copyAndChmod(self.ieprofile, ieprofile_path)



class WinXpInstaller(Win32Installer):
  """ Installer for WinXP, extends Win32Installer. """
  def __init__(self):
    """ Set up xp specific variables. """
    self.prepareProfiles()
    home = os.getenv('USERPROFILE')
    self.appdata_path = os.path.join(home, 'Local Settings\\Application Data')
    self.ieprofile = 'ieprofile'


  def buildPath(self):
    return self.findBuildPath('win32')


  def type(self):
    return 'WinXpInstaller'


class WinVistaInstaller(Win32Installer):
  """ Installer for Vista, extends Win32Installer. """
  def __init__(self):
    self.prepareProfiles()
    """ Set vista specific variables. """
    home = os.getenv('USERPROFILE')
    self.appdata_path = os.path.join(home, 'appdata\\local')
    self.ieprofile = 'ieprofile'


  def type(self):
    return 'WinVistaInstaller'


class MacInstaller(Installer):
  """ Installer for Mac, extends Installer. """
  def __init__(self):
    """ Set mac specific variables. """
    self.prepareProfiles()
    home = os.getenv('HOME')
    ffprofile = 'Library/Application Support/Firefox/Profiles'
    ffcache = 'Library/Caches/Firefox/Profiles'
    self.firefox = '/Applications/Firefox.app/Contents/MacOS/firefox-bin'
    self.ffprofile_path = os.path.join(home, ffprofile)
    self.ffcache_path = os.path.join(home, ffcache)
    self.ffprofile = 'ffprofile-mac'
    self.profile_arg = '-CreateProfile %s' % Installer.PROFILE
  
  
  def buildPath(self):
    return self.findBuildPath('osx')

  
  def type(self):
    return 'MacInstaller'


  def install(self):
    """ Do installation. """
    os.system('%s %s' % (self.firefox, self.profile_arg))
    self.installExtension(self.buildPath())
    self.__copyProfileCacheMac()


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
    ffprofile_cache = 'ffprofile-mac/Google Gears for Firefox'
    shutil.rmtree(profile_folder)
    os.mkdir(profile_folder)
    self.copyAndChmod(ffprofile_cache, gears_folder)


class LinuxInstaller(Installer):
  """ Installer for linux, extends Installer. """
  def __init__(self):
    """ Set linux specific variables. """
    self.prepareProfiles()
    home = os.getenv('HOME')
    self.ffprofile_path = os.path.join(home, '.mozilla/firefox')
    self.firefox = 'firefox'
    self.ffprofile = 'ffprofile-linux'
    self.profile_arg = '-CreateProfile %s' % Installer.PROFILE


  def buildPath(self):
    return self.findBuildPath('linux')


  def type(self):
    return 'LinuxInstaller'


  def install(self):
    """ Do installation. """
    os.system('%s %s' % (self.firefox, self.profile_arg))
    self.installExtension(self.buildPath())
