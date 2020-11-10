#pragma once

#include "Types.h"

class Sphere : public SceneElement
{
private:
    /// Положение центра сферы в пространстве
    math::Vec3<float> position_;
    /// Радиус сферы
    float radius_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    Sphere():
    SceneElement(),position_({0.0f,0.0f,0.0f}),radius_(1.0f){};

    /**
     * \brief Основной конструктор
     * \param material Материал
     * \param position Положение центра сферы
     * \param radius Радиус сферы
     */
    Sphere(const Material& material, const math::Vec3<float>& position, const float& radius):
    SceneElement(material),position_(position),radius_(radius){}

    /**
     * \brief Деструктор
     */
    ~Sphere() override = default;

    /**
     * \brief Пересечение объекта сцены и луча
     * \param ray Луч
     * \param tMin Минимальное расстояние
     * \param tMax Максимальное расстояние
     * \param tOut Итоговое расстояние до точки пересечения
     * \param normalOut Нормаль поверхности в точке пересечени
     * \return Было ли пересечение с объектом
     */
    bool intersectsRay(const math::Ray& ray, float tMin, float tMax, float* tOut, math::Vec3<float>* normalOut) const override
    {
        float t = 0;
        if(ray.intersectsSphere(position_,radius_,tMin,tMax,&t)){
            if(tOut != nullptr) *tOut = t;
            if(normalOut != nullptr) *normalOut = math::Normalize((ray.getOrigin() + (ray.getDirection() * t)) - position_);
            return true;
        }
        return false;
    }
};