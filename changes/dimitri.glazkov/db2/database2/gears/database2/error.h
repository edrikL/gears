#ifndef GEARS_DATABASE2_ERROR_H__
#define GEARS_DATABASE2_ERROR_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"

class GearsSQLError : public ModuleImplBaseClassVirtual {
	public:
		GearsSQLError()
			 : ModuleImplBaseClassVirtual("GearSQLError") {}

	//readonly attribute unsigned int code;
	void GetCode(JsCallContext *context);

	//readonly attribute DOMString message;
	void GetErrorMessage(JsCallContext *context);

	static GearsSQLError* Create(ModuleImplBaseClass* sibling, 
															 int code, 
															 std::string16 message) {
		 GearsSQLError *error = CreateModule<GearsSQLError>(sibling->GetJsRunner());
		 error->InitBaseFromSibling(sibling);

		 error->code_ = code;
		 error->message_ = message;

		 return error;
	}

	private:
		int code_;
		std::string16 message_;

		DISALLOW_EVIL_CONSTRUCTORS(GearsSQLError);
};

#endif // GEARS_DATABASE2_ERROR_H__
