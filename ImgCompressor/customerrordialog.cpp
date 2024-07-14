#include "customerrordialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>


/////////////////////////////////
//
//  class ErrorDialog
//

CustomErrorDialog::CustomErrorDialog(QWidget *parent, const QString& title, const QString& msg) :
    QDialog(parent)
{
    setWindowTitle(title);

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel      *label = new QLabel(msg, this);
    QPushButton *okButton = new QPushButton("Ok", this);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    // Add unique feedback on button press
    connect(okButton, &QPushButton::pressed, [okButton]() {
        okButton->setStyleSheet("background-color: #B0B0B0;");
    });
    connect(okButton, &QPushButton::released, [okButton]() {
        okButton->setStyleSheet("");
    });

    layout->addWidget(label);
    layout->addWidget(okButton);

    setLayout(layout);
}
