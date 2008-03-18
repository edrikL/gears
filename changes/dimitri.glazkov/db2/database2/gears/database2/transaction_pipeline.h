#ifndef GEARS_DATABASE2_TRANSACTION_PIPELINE_H__
#define GEARS_DATABASE2_TRANSACTION_PIPELINE_H__

#include <vector>

#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/message_queue.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// to avoid mkdepend.py recursion
class GearsDatabase2;
class GearsSQLTransaction;
class Statement;

const int kStatementQueueMessageType = 99;

class TransactionPipeline {
public:
	TransactionPipeline(GearsDatabase2 const *database)
		: database_(database) {}

	void Add(GearsSQLTransaction *tx);

	void RunSteps(GearsSQLTransaction *tx);

private:
  GearsDatabase2 const *database_;
	std::vector<GearsSQLTransaction*> queue_;

	//----------------------------------------------------------------------------
	// Item that is passed around through the queue
	//----------------------------------------------------------------------------
	class StatementQueueItem : public MessageData {
	public:
		StatementQueueItem(GearsDatabase2* database, 
											 GearsSQLTransaction* tx,
											 Statement* statement)
			: MessageData(), database_(database), tx_(tx), statement_(statement),
          processed_(false) {}

    bool IsProcessed() { return processed_; }

    void ProcessStatement();
    void ProcessCallback();

	private:
    bool processed_;
		GearsDatabase2 *database_;
		GearsSQLTransaction *tx_;
		Statement *statement_;
	};

	//----------------------------------------------------------------------------
	// Handler of Statement Message Queue
	//----------------------------------------------------------------------------
	class StatementQueueHandler : public ThreadMessageQueue::HandlerInterface {
	public:
		static void Init();
		virtual void HandleThreadMessage(int message_type, 
																		 MessageData *message_data);
	private:
		static bool initialized_;
		static StatementQueueHandler instance_;

	};

  DISALLOW_EVIL_CONSTRUCTORS(TransactionPipeline);
};

#endif // GEARS_DATABASE2_TRANSACTION_PIPELINE_H__
