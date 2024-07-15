#pragma once

#include <QQmlComponent>
#include <QQmlEngine>


#define SHOW_ERROR_MSG(__header_str__, __msg_text__, __p_engine__, __p_parent__)          \
    CustomErrorDialog *_p_dlg_ = new CustomErrorDialog(__p_engine__, __p_parent__);       \
    _p_dlg_->show(__header_str__, __msg_text__); // The dialog will clean itself up and delete itself when closed


class CustomErrorDialog : public QObject
{
    Q_OBJECT

public:

    explicit        CustomErrorDialog(QQmlEngine *pQMLEngine, QObject *pParentObj = nullptr);
    virtual         ~CustomErrorDialog();

    void            show(const QString& title, const QString& msg);

private:

    void            cleanup();

private:

    QQmlEngine     *m_pQMLEngine;
    QQmlComponent  *m_pComponent;
    QObject        *m_pDialogObject;
};
