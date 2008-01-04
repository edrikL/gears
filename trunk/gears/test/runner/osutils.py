# Common methods for determining what type of system is running.

import os

def osIsWin():
  if os.name == 'nt':
    return True
  else:
    return False


def osIsNix():
  if os.name == 'posix':
    return True
  else:
    return False

  
def osIsVista():
  home = os.getenv('USERPROFILE')
  appdata = os.path.join(home, 'appdata')
  if os.path.exists(appdata):
    return True
  else:
    return False


def osIsMac():
  apps = '/Applications'
  if os.path.exists(apps):
    return True
  else:
    return False    
