#pragma once

#include <QString>

#include "app/AppTypes.h"

class QPainter;
class QRectF;

namespace resultCardTheme {

QString aiTextColorForStyle(ResultCardStyle style);
QString aiGridLineColorForStyle(ResultCardStyle style);
QString etymologyHighlightColorForStyle(ResultCardStyle style);
QString styleSheetForStyle(ResultCardStyle style);

void paintCardBackground(QPainter* painter, const QRectF& cardRect, ResultCardStyle style);

} // namespace resultCardTheme
