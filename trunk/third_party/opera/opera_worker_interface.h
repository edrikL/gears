#ifndef OPERA_WORKER_INTERFACE_H__
#define OPERA_WORKER_INTERFACE_H__

// This ifdef is introduced to be able to compile this file without changes
// in both Opera and Gears. EXTERNAL_GEARS_SUPPORT will be defined in Opera.
#ifdef EXTERNAL_GEARS_SUPPORT
#include "modules/ns4plugins/src/plug-inc/npapi.h"  // for NPP for Opera
#else
#include "third_party/npapi/nphostapi.h"  // for NPP for Gears
#endif

// Interface for the Opera native worker thread implementation to
// communicate with the Gears plugin
class OperaWorkerThreadInterface {
 public:
  virtual ~OperaWorkerThreadInterface(){}

  // Will be called when Opera wants to get permanently delete
  // the worker thread
  virtual void Destroy() = 0;

  // Called from Opera when we want to create a new worker thread.
  // instance will be the plugin context from which the creation
  // was triggered.
  // global_object is the new global object created for the worker
  // url is the URL which the worker will be created from
  virtual NPObject* CreateWorkerFactory(NPP instance,
                                        NPObject *global_object,
                                        const unsigned short *url) = 0;

  // Is called when Opera needs to suspend the creation of the
  // worker thread for some reason
  virtual void SuspendObjectCreation() = 0;

  // Is called when Opera will resume the creation of the worker thread
  virtual void ResumeObjectCreationAndUpdatePermissions() = 0;
};

#endif  // OPERA_WORKER_INTERFACE_H__
