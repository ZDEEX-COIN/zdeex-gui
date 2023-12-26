// Copyright 2019-2022 The Hush developers
// Released under the GPLv3
#include "fillediconlabel.h"
#include "settings.h"
#include "guiconstants.h"

FilledIconLabel::FilledIconLabel(QWidget* parent) :
    QLabel(parent) {
    this->setMinimumSize(1, 1);
    setScaledContents(false);
}

void FilledIconLabel::setBasePixmap(QPixmap pm) {
    basePm = pm;
}

/**
 * When resized, we re-draw the whole pixmap, resizing it as needed. 
 */ 
void FilledIconLabel::resizeEvent(QResizeEvent*) {    
    QSize sz = size();  
    
    QPixmap scaled = basePm.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QString theme_name = Settings::getInstance()->get_theme_name();
    QColor color;
    if (theme_name == "blue"){
        color = COLOR_BLUE_BG;
    }else if(theme_name == "light"){
        color = COLOR_LIGHT_BG;
    }else if(theme_name == "dark"){
        color = COLOR_DARK_BG;
    }else if(theme_name =="midnight"){
        color = COLOR_MIDNIGHT_BG;
    }else if(theme_name =="zdeex"){
        color = COLOR_ZDEEX_BG;
    }else{
        color = COLOR_DEFAULT_BG;
    }

    QPixmap p(sz);
    p.fill(color);

    QPainter painter(&p);
    painter.drawPixmap((sz.width() - scaled.width()) / 2, (sz.height() - scaled.height()) / 2, scaled);
    
    QLabel::setPixmap(p);
}
