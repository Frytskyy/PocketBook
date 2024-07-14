#pragma once

#include <QDialog>


class CustomErrorDialog : public QDialog
{
public:

    CustomErrorDialog(QWidget *parent, const QString& title, const QString& msg);
};
