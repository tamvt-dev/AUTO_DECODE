#ifndef ARG_EDITOR_DIALOG_H
#define ARG_EDITOR_DIALOG_H

#include <QDialog>
#include <QJsonObject>

class ArgEditorDialog : public QDialog
{
public:
    ArgEditorDialog(const QJsonObject &op, QWidget *parent = nullptr);
    QJsonObject getArgs() const;

private:
    QJsonObject m_args;
};

#endif

