#ifndef _SCENERY3DDIALOG_HPP_
#define _SCENERY3DDIALOG_HPP_

#include "StelDialog.hpp"
#include "ui_scenery3dDialog.h"

class Scenery3dMgr;

class Scenery3dDialog : public StelDialog
{
    Q_OBJECT
public:
    Scenery3dDialog(QObject* parent = NULL);
	~Scenery3dDialog();

public slots:
        void retranslate();

protected:
	void createDialogContent();

private slots:
	void on_comboBoxShadowFiltering_currentIndexChanged(int index);
    void scenery3dChanged(QListWidgetItem* item);

    //! Update the widget to make sure it is synchrone if a value was changed programmatically
    //! This is called automatically from the signals in the manager class
    void updateFromManager();

private:
    Ui_scenery3dDialogForm* ui;
    Scenery3dMgr* mgr;
};

#endif
