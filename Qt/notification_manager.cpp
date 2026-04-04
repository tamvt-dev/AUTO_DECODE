#include "notification_manager.h"
#include <QWidget>

NotificationManager& NotificationManager::instance()
{
    static NotificationManager manager;
    return manager;
}

NotificationManager::NotificationManager()
    : m_parentWidget(nullptr)
{
}

void NotificationManager::setParentWidget(QWidget *parent)
{
    m_parentWidget = parent;
}

void NotificationManager::showNotification(const QString &message, MessageType type, int timeout)
{
    if (!m_parentWidget) {
        return;
    }

    // Remove oldest notification if queue is full
    if (m_activeNotifications.size() >= MAX_TOASTS) {
        NotificationWidget *old = m_activeNotifications.dequeue();
        old->animateOut();
    }

    // Create new notification
    NotificationWidget::NotificationType widgetType;
    switch (type) {
        case Success: widgetType = NotificationWidget::Success; break;
        case Error:   widgetType = NotificationWidget::Error; break;
        case Warning: widgetType = NotificationWidget::Warning; break;
        case Info:
        default:      widgetType = NotificationWidget::Info; break;
    }

    NotificationWidget *notification = new NotificationWidget(message, widgetType, timeout, m_parentWidget);
    m_activeNotifications.enqueue(notification);

    connect(notification, &NotificationWidget::finished, this, &NotificationManager::onNotificationFinished);

    positionNotification(notification);
    notification->show();
}

void NotificationManager::positionNotification(NotificationWidget *widget)
{
    if (!m_parentWidget) return;

    int y = 15;

    // Stack notifications vertically
    for (NotificationWidget *existing : m_activeNotifications) {
        if (existing != widget && existing->isVisible()) {
            y = existing->geometry().bottom() + SPACING;
        }
    }

    // Set the target Y position for the widget's animation
    widget->setTargetY(y);
}

void NotificationManager::onNotificationFinished()
{
    NotificationWidget *notification = qobject_cast<NotificationWidget*>(sender());
    if (notification) {
        m_activeNotifications.removeAll(notification);
        notification->deleteLater();
    }
}

void NotificationManager::clear()
{
    while (!m_activeNotifications.isEmpty()) {
        NotificationWidget *notification = m_activeNotifications.dequeue();
        notification->animateOut();
    }
}
