// ========================================================================= //
// Authors: Roman Getto, Matthias Bein                                       //
// mailto:roman.getto@gris.informatik.tu-darmstadt.de                        //
//                                                                           //
// GRIS - Graphisch Interaktive Systeme                                      //
// Technische Universität Darmstadt                                          //
// Fraunhoferstrasse 5                                                       //
// D-64283 Darmstadt, Germany                                                //
//                                                                           //
// ========================================================================== //

#include "vec3.h"

template<class T>
class Ray {

  typedef Vec3<T> Vec3_;

 public:
	 	 
  Vec3_ d,o;

  Ray(const Vec3<T>& origin, const Vec3<T>& p) : o(origin),
    d(p.x() - origin.x(), p.y() - origin.y(), p.z() - origin.z()) {
      d.normalize();
  }

  Ray(const T origin[3], const T p[3]): o(origin[0], origin[1], origin[2]),
    d(p[0] - origin[0], p[1] - origin[1], p[2] - origin[2]) {
      d.normalize();
    }

  bool triangleIntersect(const Vec3_& p0, const Vec3_& p1, const Vec3_& p2, T& u, T& v, T &rayt) const
   {
     Vec3_ e1(p1[0]-p0[0],
	         p1[1]-p0[1],
	         p1[2]-p0[2]);

     Vec3_ e2(p2[0]-p0[0],
	         p2[1]-p0[1],
	         p2[2]-p0[2]);

     Vec3_ t(o[0]-p0[0],
	        o[1]-p0[1],
	        o[2]-p0[2]);

     Vec3_ p = cross(d, e2);
     Vec3_ q = cross(t, e1);

     T d1 = p*e1;
     if (std::abs(d1) < 10e-7)
       return false;

     T f = 1.0f/d1;

     u = f*(p*t);

     if (u < 0 || u > 1.0)
       return false;

     v = f*(q*d);

     if (v < 0.0 || v > 1.0 || (u+v) > 1.0)
       return false;

     rayt = f*(q*e2);

     return true;
   }


};
