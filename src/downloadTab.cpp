/* downloadTab.cpp
//
// Written by Gijs de Rooy, started March 2010.
//
// Copyright (C) 2010-2013  Gijs de Rooy
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id: $
   ==================================================================== */


//################################################################//
//################################################################//
//#########      DOWNLOAD SHAPEFILES FROM MAPSERVER      #########//
//################################################################//
//################################################################//

void MainWindow::on_downloadShapefilesButton_clicked()
{
#ifdef Q_OS_WIN
    QFile f("7z.exe");
    if ( !f.exists() ) {
        QString msg = "Unable to locate "+QDir::currentPath()+"/7z.exe";
        QMessageBox::critical(this,"File not found", msg);
        return;
    }
#endif

    double southInt    = ui->minLatField->text().toDouble();
    double northInt    = ui->maxLatField->text().toDouble();
    double westInt     = ui->minLonField->text().toDouble();
    double eastInt     = ui->maxLonField->text().toDouble();

    if ((westInt < eastInt) && (northInt > southInt)) {
        double squareDegree = abs((eastInt-westInt)*(northInt-southInt));
        if (squareDegree <= 144) {
            //== Set server - maybe MAP_SERVER_URL as constant
            QUrl url("http://mapserver.flightgear.org/dlshp?");

            //== Set Source Dir
            QString source = ui->shapefilesSourceSelect->currentText();
            QString layer;
            if (source == "Custom scenery"){
                layer = "cs";

            }else if (source == "OpenStreetMap"){
                layer = "osm";

            }else if (source == "CORINE 2000 (Europe)"){
                layer = "clc00v16";

            }else {
                layer = "clc06v16";
            }

            //== add Query vars
            url.addQueryItem("layer", layer);
            url.addQueryItem("xmin", m_west);
            url.addQueryItem("xmax", m_east);
            url.addQueryItem("ymin", m_south);
            url.addQueryItem("ymax", m_north);

            //= save output to log
            GUILog( url.toString() + "\n", "download" );

            // reset progressbar
            ui->downloadShapefilesProgressBar->setMinimum(0);
            ui->downloadShapefilesProgressBar->setMaximum(0);

            // disable button during download
            ui->downloadShapefilesButton->setEnabled(0);
            ui->downloadShapefilesButton->setText("Connecting to server...");

            if ( ! _manager->get(QNetworkRequest(url)) ) {
                // TODO: Open internal webbrowser
                // QWebView::webview = new QWebView;
                // webview.load(url);
                QMessageBox::critical(this, "URL cannot be opened","The following URL cannot be opened " + url.toString() +".\nCopy the URL to your browser");
            }
        } else {
            QMessageBox::critical(this,"Boundary error","The selected area is too large. The download server does not accept areas larger than 144 square degrees. Your area spans "+QString::number(squareDegree)+" square degrees.");
        }
    }
    else{
        QString msg = tr("Minimum longitude and/or latitude is not less than maximum.\nCorrect the coordinates, and try again.");
        GUILog( msg + "\n", "download" );
        QMessageBox::critical(this,tr("Boundary error"),msg);
    }

}


//################################################################//
//################################################################//
//##############   DOWNLOAD ELEVATION DATA SRTM    ###############//
//################################################################//
//################################################################//

void MainWindow::on_downloadElevationButton_clicked()
{

    double latMin = ui->minLatField->text().toDouble();
    double latMax = ui->maxLatField->text().toDouble();
    double lonMin = ui->minLonField->text().toDouble();
    double lonMax = ui->maxLonField->text().toDouble();

    QString tileLat;
    QString tileLon;

    // reset progress bar
    ui->downloadElevationProgressBar->setValue(0);
    double totalDouble = (latMax-latMin)*(lonMax-lonMin);
    QString totalString;
    totalString.sprintf("%.0f", totalDouble);
    int totalTiles = totalString.toInt();
    if (totalTiles == 0)
        totalTiles = 1;
    ui->downloadElevationProgressBar->setMaximum(totalTiles);

    // disable button during download
    ui->downloadElevationButton->setEnabled(0);

    QString sourceElev = ui->elevationSourceSelect->currentText();
    QString urlElev;
    if (sourceElev == "usgs.gov (SRTM-1)"){
        urlElev = "https://dds.cr.usgs.gov/srtm/version2_1/SRTM1/";
    } else if (sourceElev == "usgs.gov (SRTM-3)"){
        urlElev = "https://dds.cr.usgs.gov/srtm/version2_1/SRTM3/";

    }

    if (lonMin < 0)
        lonMin = lonMin - 1;
    if (lonMax < 0)
        lonMax = lonMax - 1;
    if (latMin < 0)
        latMin = latMin - 1;
    if (latMax < 0)
        latMax = latMax - 1;
    for (int lat=static_cast<int>(latMin); lat<=static_cast<int>(latMax); lat++) {
        for (int lon=static_cast<int>(lonMin); lon<=static_cast<int>(lonMax); lon++) {
            QString tile = "";
            if (lat < 0) {
                tile += "S";
            } else {
                tile += "N";
            }
            tileLat.sprintf("%02d",abs(lat));
            tile += tileLat;
            if (lon < 0) {
                tile += "W";
            } else {
                tile += "E";
            }
            tileLon.sprintf("%03d",abs(lon));
            tile += tileLon;

            QList<QString> folders;
            if (sourceElev.contains("SRTM-1")) {
                folders << "Region_01" << "Region_02" << "Region_03" << "Region_04" << "Region_05" << "Region_06" << "Region_07";
            } else {
                folders << "Africa" << "Australia" << "Eurasia" << "Islands" << "North_America" << "South_America";
            }
            int i = 0;
            bool succes = 0;
            while (!succes && i < 6) {
                QUrl url(urlElev+folders.at(i)+"/"+tile+".hgt.zip");
                QNetworkReply *reply = _manager->get(QNetworkRequest(url));
                if (reply->error()) {
                    GUILog( url.toString() + "\n", "download" );
                    succes = 1;
                }
                i++;
            }
        }
    }
}

// update elevation download range
void MainWindow::updateElevationRange()
{
    QString east;
    QString north;
    QString south;
    QString west;

    double eastDbl     = ui->maxLonField->text().toDouble();
    double northDbl    = ui->maxLatField->text().toDouble();
    double southDbl    = ui->minLatField->text().toDouble();
    double westDbl     = ui->minLonField->text().toDouble();

    // initialize text colors
    QPalette q1;
    QPalette q2;
    q1.setColor(QPalette::Text, Qt::black);
    q2.setColor(QPalette::Text, Qt::black);

    QString prevmin = minElev;
    QString prevmax = maxElev;

    // CAN we invalidate boundaries here... first..

    // sorry mate, your outa bounds..

    // check if boundaries are valid
    if (westDbl < eastDbl && northDbl > southDbl){

        // clear the strings
        minElev = "";
        maxElev = "";

        // max north
        if (northDbl >= 0){
            maxElev += "N";
        }
        if (northDbl < 0) {
            maxElev += "S";
            northDbl = northDbl-1;
        }
        north.sprintf("%02d", abs(northDbl));
        maxElev += north;

        // max south
        if (southDbl >= 0){
            minElev += "N";
        }
        if (southDbl < 0) {
            minElev += "S";
            southDbl = southDbl-1;
        }
        south.sprintf("%02d", abs(southDbl));
        minElev += south;

        // max east
        if (eastDbl >= 0){
            maxElev += "E";
        }
        if (eastDbl < 0) {
            maxElev += "W";
            eastDbl = eastDbl-1;
        }
        east.sprintf("%03d", abs(eastDbl));
        maxElev += east;

        //max west
        if (westDbl >= 0){
            minElev += "E";
        }
        if (westDbl < 0) {
            minElev += "W";
            westDbl = westDbl-1;
        }
        west.sprintf("%03d", abs(westDbl));
        minElev += west;

        // output elevation range chosen in left
        ui->minDownloadRangeLabel->setText(minElev);
        ui->maxDownloadRangeLabel->setText(maxElev);

        if ((prevmin != minElev) || (prevmax != maxElev)) {
            elevList = ""; // restart SRTM elevation
            // build a HELPFUL SRTM list to add to the LOG, if enabled (or not)
            for (double ew = westDbl; ew <= eastDbl; ew += 1.0) {
                for (double ns = southDbl; ns <= northDbl; ns += 1.0) {
                    east.sprintf("%03d", abs(ew));
                    north.sprintf("%02d", abs(ns));
                    if (elevList.size()) elevList += ";"; // add separator
                    if (ns < 0) {
                        elevList += "S";
                    } else {
                        elevList += "N";
                    }
                    elevList += north;
                    if (ew < 0) {
                        elevList += "W";
                    } else {
                        elevList += "E";
                    }
                    elevList += east;
                }
            }
            //GUILog( elevList, "download" ); /* provide a 'helpful' list of SRTM files */
        }

        // enable download buttons
        ui->downloadShapefilesButton->setEnabled(1);
        ui->downloadElevationButton->setEnabled(1);
    }

    // if boundaries are not valid: do not display elevation range and set text color to red
    else{
        ui->minDownloadRangeLabel->setText("");
        ui->maxDownloadRangeLabel->setText("");

        if (westDbl == eastDbl || westDbl > eastDbl){
            q1.setColor(QPalette::Text, Qt::red);
        }
        if (northDbl == southDbl || southDbl > northDbl){
            q2.setColor(QPalette::Text, Qt::red);
        }

        // disable download buttons
        ui->downloadShapefilesButton->setDisabled(1);
        ui->downloadElevationButton->setDisabled(1);
    }

    // change text color in the boundary fields
    // TODO - change into a widget set said pedro..
    ui->maxLonField->setPalette(q1);
    ui->minLonField->setPalette(q1);
    ui->maxLatField->setPalette(q2);
    ui->minLatField->setPalette(q2);
}

void MainWindow::addZoomButtons()
{
    // create buttons as controls for zoom
    QPushButton* zoomin = new QPushButton("+");
    QPushButton* zoomout = new QPushButton("-");
    QPushButton* pan = new QPushButton("Select area");
#define COPY QChar(169)
    QLabel* copyright = new QLabel(QString("%1 <a href=\"http://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors").arg(COPY));
    copyright->setOpenExternalLinks(true);
    copyright->setStyleSheet("QLabel { background-color: rgba(240,240,240,220);}");
    zoomin->setMaximumWidth(47);
    zoomout->setMaximumWidth(47);
    pan->setCheckable(true);
    pan->setMaximumWidth(100);

    connect(zoomin, SIGNAL(clicked(bool)),
            mc, SLOT(zoomIn()));
    connect(zoomout, SIGNAL(clicked(bool)),
            mc, SLOT(zoomOut()));
    connect(pan, SIGNAL(toggled(bool)),
            mc, SLOT(buttonToggled(bool)));
    // add zoom buttons to the layout of the MapControl
    QVBoxLayout* innerlayout = new QVBoxLayout;
    QVBoxLayout* innerlayoutV = new QVBoxLayout;
    QHBoxLayout* innerlayoutH = new QHBoxLayout;
    QHBoxLayout* outerlayoutH = new QHBoxLayout;
    innerlayoutH->addWidget(zoomin);
    innerlayoutH->addWidget(zoomout);
    innerlayoutH->addStretch(1);
    innerlayout->addLayout(innerlayoutH);
    innerlayout->addWidget(pan);
    innerlayout->addStretch(1);
    outerlayoutH->addLayout(innerlayout);
    outerlayoutH->addStretch(1);
    innerlayoutV->addStretch(1);
    innerlayoutV->addWidget(copyright);
    outerlayoutH->addLayout(innerlayoutV);
    mc->setLayout(outerlayoutH);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (ui->tabWidget->currentIndex() == 1) {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;
        if (event->orientation() == Qt::Vertical) {
            if (numSteps > 0) {
                mc->zoomIn();
            } else {
                mc->zoomOut();
            }
        }
        event->accept();
    }
}

void MainWindow::draggedRect(QRectF rect)
{
    QList<QPointF> coords;
    coords.append(rect.topLeft());
    coords.append(rect.bottomRight());
    mc->setViewAndZoomIn(coords);

    double latminpoint = rect.bottom();
    double latmaxpoint = rect.top();
    double lonminpoint = rect.left();
    double lonmaxpoint = rect.right();

    if ( latminpoint > latmaxpoint ) switchPoint( latminpoint, latmaxpoint );
    if ( lonminpoint > lonmaxpoint ) switchPoint( lonminpoint, lonmaxpoint );

    ui->minLatField->setText(QString::number(latminpoint));
    ui->maxLatField->setText(QString::number(latmaxpoint));
    ui->minLonField->setText(QString::number(lonminpoint));
    ui->maxLonField->setText(QString::number(lonmaxpoint));

    updateArea();

    QList<Point*> points;
    points.append(new Point(lonminpoint, latminpoint, "1"));
    points.append(new Point(lonminpoint, latmaxpoint, "1"));
    points.append(new Point(lonmaxpoint, latmaxpoint, "1"));
    points.append(new Point(lonmaxpoint, latminpoint, "1"));
    points.append(new Point(lonminpoint, latminpoint, "1"));
    QPen* linepen = new QPen(Qt::red);
    linepen->setWidth(2);
    LineString* ls = new LineString(points, "Boundary Area", linepen);
    mainlayer->clearGeometries();
    mainlayer->addGeometry(ls);
}


void MainWindow::updateArea()
{
    m_north = ui->maxLatField->text();
    m_south = ui->minLatField->text();
    m_east  = ui->maxLonField->text();
    m_west  = ui->minLonField->text();

    if (!m_west.isEmpty()) {
        QList<QPointF> coord;
        coord.append(QPointF(m_west.toFloat(), m_north.toFloat()));
        coord.append(QPointF(m_east.toFloat(), m_south.toFloat()));
        mc->setViewAndZoomIn(coord);

        QList<Point*> points;
        points.append(new Point(m_west.toFloat(), m_south.toFloat(), "1"));
        points.append(new Point(m_west.toFloat(), m_north.toFloat(), "1"));
        points.append(new Point(m_east.toFloat(), m_north.toFloat(), "1"));
        points.append(new Point(m_east.toFloat(), m_south.toFloat(), "1"));
        points.append(new Point(m_west.toFloat(), m_south.toFloat(), "1"));
        QPen* linepen = new QPen(Qt::red);
        linepen->setWidth(2);
        LineString* ls = new LineString(points, "Area Boundary", linepen);
        mainlayer->clearGeometries();
        mainlayer->addGeometry(ls);
    }

    settings.setValue("boundaries/north", m_north);
    settings.setValue("boundaries/south", m_south);
    settings.setValue("boundaries/east", m_east);
    settings.setValue("boundaries/west", m_west);

    updateElevationRange();
    updateCenter();
}

void MainWindow::switchPoint(double& a, double& b)
{
    double temp(a); //Save the value to 'a'
    a = b;          //Replace the value of 'a' with that of 'b'
    b = temp;       //Place the temporary value of the original 'a' in 'b'
}

void MainWindow::on_maxLonField_editingFinished()
{
    updateArea();
}

void MainWindow::on_minLonField_editingFinished()
{
    updateArea();
}

void MainWindow::on_maxLatField_editingFinished()
{
    updateArea();
}

void MainWindow::on_minLatField_editingFinished()
{
    updateArea();
}
