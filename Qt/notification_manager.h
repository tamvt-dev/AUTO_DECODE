#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <QString>
#include <QQueue>
#include <QObject>
#include "notification_widget.h"

class QWidget;

class NotificationManager : public QObject
{
    Q_OBJECT

public:
    static NotificationManager& instance();

    enum MessageType {
        Success,
        Error,
        Info,
        Warning
    };

    void setParentWidget(QWidget *parent);
    void showNotification(const QString &message, MessageType type, int timeout = 3000);
    void clear();

private slots:
    void onNotificationFinished();

private:
    NotificationManager();
    ~NotificationManager() = default;

    void positionNotification(NotificationWidget *widget);

    QWidget *m_parentWidget;
    QQueue<NotificationWidget*> m_activeNotifications;
    static const int MAX_TOASTS = 3;
    static const int SPACING = 10;
};

#endif // NOTIFICATION_MANAGER_H
