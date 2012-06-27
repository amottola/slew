#ifndef __wizardpage_h__
#define __wizardpage_h__


#include "slew.h"

#include "wizard.h"
#include "constants/wizardpage.h"


class WizardPage_Impl : public QWizardPage, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(WizardPage)
	
	SL_DECLARE_SIZE_HINT(QWidget)
	
	virtual void initializePage();
	virtual bool isComplete() const { return fIsComplete; }
	virtual bool validatePage();
	virtual int nextId() const;
	
	void setComplete(bool complete) { fIsComplete = complete; emit completeChanged(); }
	void setNextId(int id) { fHasNextID = true; fNextID = id; setCommitPage(isCommitPage()); }
	
private:
	bool		fIsComplete;
	bool		fHasNextID;
	int			fNextID;
};


#endif
