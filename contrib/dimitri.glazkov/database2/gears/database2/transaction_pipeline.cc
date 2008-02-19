#include "gears/database2/transaction_pipeline.h"
#include "gears/database2/database2.h"
#include "gears/database2/transaction.h"
#include "gears/database2/statement.h"

void TransactionPipeline::Add(GearsSQLTransaction *tx) {
	bool first = queue_.empty();

	queue_.push_back(tx);

	if (first) {
		RunSteps(tx);
	}
}

void TransactionPipeline::RunSteps(GearsSQLTransaction *tx) {
	// for now, immediately invoke callback
	if (tx->HasCallback()) {
		tx->InvokeCallback();
	}
// TODO(dglazkov): break these up into sync segments

//Open a new SQL transaction to the database, and create a SQLTransaction object
//that represents that transaction.
//
//If an error occured in the opening of the transaction, jump to the last step.
//
//If a preflight operation was defined for this instance of the transaction 
//steps, run that. If it fails, then jump to the last step. 
//(This is basically a hook for the changeVersion() method.)
//
//Invoke the transaction callback with the aforementioned SQLTransaction object
//as its only argument.
//
//If the callback couldn't be called (e.g. it was null), or if the callback was
//invoked and raised an exception, jump to the last step.
//
//While there are any statements queued up in the transaction, perform 
//the following steps for each queued up statement in the transaction, 
//oldest first. Each statement has a statement, a result set callback, and 
//optionally an error callback.
//
//If the statement is marked as bogus, jump to the "in case of error" steps 
//below.
//
//Execute the statement in the context of the transaction. [SQL]
//
//If the statement failed, jump to the "in case of error" steps below.
//
//Create a SQLResultSet object that represents the result of the statement.
//
//Invoke the statement's result set callback with the SQLTransaction object 
//as its first argument and the new SQLResultSet object as its second argument.
//
//If the callback was invoked and raised an exception, jump to the last step 
//in the overall steps.
//
//Move on to the next statement, if any, or onto the next overall step 
//otherwise.
//
//In case of error (or more specifically, if the above substeps say to jump 
//to the "in case of error" steps), run the following substeps:
//
//If the statement had an associated error callback, then invoke that error 
//callback with the SQLTransaction object and a newly constructed SQLError 
//object that represents the error that caused these substeps to be run as the 
//two arguments, respectively.
//
//If the error callback returns false, then move on to the next statement, if 
//any, or onto the next overall step otherwise.
//
//Otherwise, the error callback did not return false, or there was no error 
//callback. Jump to the last step in the overall steps.
//
//If a postflight operation was defined for his instance of the transaction 
//steps, run that. If it fails, then jump to the last step. (This is basically
//a hook for the changeVersion() method.)
//
//Commit the transaction.
//
//If an error occured in the committing of the transaction, jump to the last 
//step.
//
//Invoke the success callback.
//
//End these steps. The next step is only used when something goes wrong.
//
//Call the error callback with a newly constructed SQLError object that 
//represents the last error to have occured in this transaction. 
//If the error callback returned false, and the last error 
//wasn't itself a failure when committing the transaction, then try to 
//commit the transaction. 
//
//If that fails, or if the callback couldn't be called (e.g. the method was 
//called with only one argument), or if it didn't return false, then 
//rollback the transaction. Any still-pending statements in the 
//transaction are discarded.
}

bool TransactionPipeline::StatementQueueHandler::initialized_ = false;
TransactionPipeline::StatementQueueHandler TransactionPipeline::StatementQueueHandler::instance_;



void TransactionPipeline::StatementQueueHandler::Init() {
  if (!initialized_) {
    initialized_ = true;
    ThreadMessageQueue::GetInstance()->
			RegisterHandler(kStatementQueueMessageType, &instance_);
  }
}

void TransactionPipeline::
			StatementQueueHandler::HandleThreadMessage(int message_type, 
																					      MessageData *message_data) {
	assert(message_type == kStatementQueueMessageType);
	// do something
}


