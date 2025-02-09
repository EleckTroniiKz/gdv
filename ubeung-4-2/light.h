//
// Created by danielr on 14.01.21.
//

#ifndef UEBUNG_04_LIGHT_H
#define UEBUNG_04_LIGHT_H

#include "vec3.h"

struct Light {
    Vec3f position;
    float ambientIntensity;
    float lightIntensity;
};

#endif //UEBUNG_04_LIGHT_H
