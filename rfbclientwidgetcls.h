/*
 * Copyright (C) 2013 Azril Azam
 * <azrilazam@gmail.com>
 *
 * Desc
 *
 * This file is a part of the 100% Qt5 VNC RFB-CLIENT implementation without
 * using any 3rd party rfb library
 *
 * rfbclientwidgetcls.h defines all constant and functions that are required
 * to operates the client as widget container to display the rendered graphic bits
 * and to accept keyboard + mouse IO
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

#ifndef RFBCLIENTWIDGETCLS_H
#define RFBCLIENTWIDGETCLS_H

#include <QWidget>
#include <QPainter>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QApplication>
#include "rfbclientcls.h"
#include <QPixmap>
#include <QCursor>

class rfbclientwidgetcls : public QWidget
{
    Q_OBJECT
public:
    explicit rfbclientwidgetcls(QWidget *parent = 0);
    ~rfbclientwidgetcls();

    bool connectVNCTCP(QString server, qint16 port=5900);
    bool connectVNCIPC(QString server);
    void disconnectVNC();
    bool setPauseRFB();
    bool setResumeRFB();
    QImage getScreenCapture();


private:
    rfbclientcls vncClient;
    QPixmap vncPix;
    QImage vncImg;

    quint16 updateX, updateY;
    bool vncConnected;
    quint8 mouseButtonPressed;
    qint16 mouseX,mouseY;
    QCursor myCursor;

    qint16 oriRFBWidth, oriRFBHeight;

    bool holdCTRLKey;
    bool screenGrab;
    QPoint globalPos;

    QPoint widgetGlobalPos, cursorGlobalPos;

    bool lockedVNC;
    bool alreadyHasLockedImage;

    void grabIO();
    void ungrabIO();

signals:
    void vncFrameResizeSignal();
    void vncFrameGetFocusSignal();
    void rfbClientConnectedSignal();
    void rfbClientDisconnectedSignal();
    void rfbClientPauseSignal();
    void vncGrabIOSIG();
    void vncUngrabIOSIG();

private slots:
    void vncResizeSlot(qint16 width, qint16 height,QString serverVNCName);
    void vncFrameBufferUpdateSlot();
    void vncHostConnectedSlot();
    void vncHostDisconnectedSlot();
    void vncHostPauseSlot();
    //void vncHostCursorPositionSlot(qint32 x, qint32 y);

protected:
    void paintEvent(QPaintEvent *);
    void closeEvent (QCloseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void resizeEvent(QResizeEvent *);
    //void mouseDoubleClickEvent(QMouseEvent *e);

public slots:

};

#endif // RFBCLIENTWIDGETCLS_H
