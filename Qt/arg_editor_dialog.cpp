#include "arg_editor_dialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QJsonObject>

ArgEditorDialog::ArgEditorDialog(const QJsonObject &op, QWidget *parent)
    : QDialog(parent), m_args(op["args"].toObject())
{
    setWindowTitle(op["name"].toString() + " - Edit Arguments");
    setMinimumWidth(300);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel(op["name"].toString());
    title->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(title);

    // For AES example - key field
    if (op["name"].toString().contains("AES", Qt::CaseInsensitive)) {
        QLabel *keyLabel = new QLabel("Key (hex):");
        QLineEdit *keyEdit = new QLineEdit;
        keyEdit->setText(m_args["key"].toString());
        layout->addWidget(keyLabel);
        layout->addWidget(keyEdit);

        connect(keyEdit, &QLineEdit::textChanged, [this, keyEdit]() {
            m_args["key"] = keyEdit->text();
        });
    }

    // Generic args for other ops
    QJsonObject::const_iterator it = m_args.constBegin();
    while (it != m_args.constEnd()) {
        QLabel *label = new QLabel(QString("%1:").arg(it.key()));
        QLineEdit *edit = new QLineEdit(it.value().toString());
        layout->addWidget(label);
        layout->addWidget(edit);

        QString key = it.key();
        connect(edit, &QLineEdit::textChanged, [this, key](const QString &text) {
            m_args[key] = text;
        });
        ++it;
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QJsonObject ArgEditorDialog::getArgs() const
{
    return m_args;
}

