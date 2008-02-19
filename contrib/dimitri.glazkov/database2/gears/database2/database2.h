#ifndef GEARS_DATABASE2_DATABASE2_H__
#define GEARS_DATABASE2_DATABASE2_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"

#include "gears/database2/transaction_pipeline.h"

class GearsDatabase2 : public ModuleImplBaseClassVirtual {
 public:
   GearsDatabase2()
       : ModuleImplBaseClassVirtual("GearsDatabase2") {}

	// transaction(...)
  // IN: SQLTransactionCallback callback, 
	//		 optional SQLTransactionErrorCallback errorCallback, 
	//     optional VoidCallback successCallback);
  // OUT: void
  void Transaction(JsCallContext *context);

	static GearsDatabase2* Create(ModuleImplBaseClass *sibling) {
		GearsDatabase2* database = 
			CreateModule<GearsDatabase2>(sibling->GetJsRunner());
		database->InitBaseFromSibling(sibling);
		database->pipeline_.reset(new TransactionPipeline(database));
		return database;
	}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GearsDatabase2);
	scoped_ptr<TransactionPipeline> pipeline_;
};

#endif // GEARS_DATABASE2_DATABASE2_H__
