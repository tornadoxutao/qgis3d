#include "tessellator.h"

#include "qgscurve.h"
#include "qgspoint.h"
#include "qgspolygon.h"

#include "poly2tri/poly2tri.h"

#include <QtDebug>

#include <QVector3D>

static void make_quad(float x0, float y0, float x1, float y1, float zLow, float zHigh, QVector<float>& data, bool addNormals)
{
  float dx = x1-x0;
  float dy = -(y1-y0);

  // perpendicular vector in plane to [x,y] is [-y,x]
  QVector3D vn(-dy, 0, dx);
  vn.normalize();

  // triangle 1
  data << x0 << zHigh << -y0;
  if (addNormals)
    data << vn.x() << vn.y() << vn.z();
  data << x1 << zHigh << -y1;
  if (addNormals)
    data << vn.x() << vn.y() << vn.z();
  data << x0 << zLow  << -y0;
  if (addNormals)
    data << vn.x() << vn.y() << vn.z();

  // triangle 2
  data << x0 << zLow  << -y0;
  if (addNormals)
    data << vn.x() << vn.y() << vn.z();
  data << x1 << zHigh << -y1;
  if (addNormals)
    data << vn.x() << vn.y() << vn.z();
  data << x1 << zLow  << -y1;
  if (addNormals)
    data << vn.x() << vn.y() << vn.z();
}


Tessellator::Tessellator(double originX, double originY, bool addNormals)
  : originX(originX)
  , originY(originY)
  , addNormals(addNormals)
{
  stride = 3*sizeof(float);
  if (addNormals)
    stride += 3*sizeof(float);
}


static bool _isRingCounterClockWise(const QgsCurve& ring)
{
  double a = 0;
  int count = ring.numPoints();
  QgsVertexId::VertexType vt;
  QgsPoint pt, ptPrev;
  ring.pointAt(0, ptPrev, vt);
  for (int i = 1; i < count + 1; ++i)
  {
    ring.pointAt(i % count, pt, vt);
    a += ptPrev.x() * pt.y() - ptPrev.y() * pt.x();
    ptPrev = pt;
  }
  return a > 0; // clockwise if a is negative
}

void Tessellator::addPolygon(const QgsPolygonV2 &polygon, float height, float extrusionHeight)
{
  const QgsCurve* exterior = polygon.exteriorRing();

  std::vector<p2t::Point*> polyline;
  polyline.reserve(exterior->numPoints());

  QgsVertexId::VertexType vt;
  QgsPoint pt;

  for (int i = 0; i < exterior->numPoints() - 1; ++i)
  {
    exterior->pointAt(i, pt, vt);
    polyline.push_back(new p2t::Point(pt.x() - originX, pt.y() - originY));
  }

  // TODO: holes
  // TODO: robustness (no duplicate / nearly duplicate points, ...)

  p2t::CDT* cdt = new p2t::CDT(polyline);
  cdt->Triangulate();

  std::vector<p2t::Triangle*> triangles = cdt->GetTriangles();

  for (size_t i = 0; i < triangles.size(); ++i)
  {
    p2t::Triangle* t = triangles[i];
    for (int j = 0; j < 3; ++j)
    {
      p2t::Point* p = t->GetPoint(j);
      data << p->x << extrusionHeight+height << -p->y;
      if (addNormals)
        data << 0.f << 1.f << 0.f;
    }
  }

  delete cdt;
  qDeleteAll(polyline);

  // add walls if extrusion is enabled
  if (extrusionHeight != 0)
  {
    // we need to find out orientation of the ring so that the triangles we generate
    // face the right direction
    bool is_counter_clockwise = _isRingCounterClockWise(*exterior);

    QgsPoint ptPrev;
    exterior->pointAt(!is_counter_clockwise ? 0 : exterior->numPoints() - 1, ptPrev, vt);
    for (int i = 1; i < exterior->numPoints(); ++i)
    {
      exterior->pointAt(!is_counter_clockwise ? i : exterior->numPoints() - i - 1, pt, vt);
      float x0 = ptPrev.x() - originX, y0 = ptPrev.y() - originY;
      float x1 = pt.x() - originX, y1 = pt.y() - originY;
      // make a quad
      make_quad(x0, y0, x1, y1, height, height+extrusionHeight, data, addNormals);
      ptPrev = pt;
    }
  }
}
