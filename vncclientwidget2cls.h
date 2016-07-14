/*
 * Copyright (C) 2013 Azril Azam
 * <azrilazam@gmail.com>
 *
 * Desc
 *
 * This file is a part of the 100% Qt5 VNC RFB-CLIENT implementation without
 * using any 3rd party rfb library
 *
 * rfbclientwidget2cls.h defines all constant and functions that are required
 * to handle the rdb widget on GUI user experiance
 *
 *
 * Qt5 RFB-CLIENT is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Qt5 RFB-CLIENT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Qt HTML platform plugin. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef VNCCLIENTWIDGET2CLS_H
#define VNCCLIENTWIDGET2CLS_H

#include <QWidget>
#include "rfbclientwidgetcls.h"

class vncclientwidget2cls : public QWidget
{
    Q_OBJECT

public:
    explicit vncclientwidget2cls(QWidget *parent = 0);
    ~vncclientwidget2cls();

    bool connectVNCTCP(QString server, qint16 port=5900);
    bool connectVNCIPC(QString server);
    void disconnectVNC();
    void updateSize();
    bool setPauseVNC();
    bool setResumeVNC();
    QImage getScreenCapture();

private:
    rfbclientwidgetcls rfbWidget;
    bool widgetResizing;

signals:

    void vncFrameResizeEvent();
    void vncClientConnectedSignal();
    void vncClientDisconnectedSignal();
    void vncClientPauseSignal();
    void vncClientScreenLockSignal();
    void vncClientScreenUnlockSignal();

public slots:

private slots:
    void vncFrameResizeSlot();
    void rfbClientConnectedSlot();
    void rfbClientDisconnectedSlot();
    void rfbClientPauseSlot();
    void rfbClientScreenLockSlot();
    void rfbClientScreenUnlockSlot();

protected:
    void resizeEvent(QResizeEvent *);
    void changeEvent(QEvent *E);
    void closeEvent(QCloseEvent *E);
};



#endif // VNCCLIENTWIDGET2CLS_H
