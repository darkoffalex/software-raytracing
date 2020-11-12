#pragma once

#include "Types.h"

class Sphere : public HittableElement
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
    HittableElement(),position_({0.0f,0.0f,0.0f}),radius_(1.0f){};

    /**
     * \brief Основной конструктор
     * \param position Положение центра сферы
     * \param radius Радиус сферы
     */
    Sphere(const math::Vec3<float>& position, const float& radius):
    position_(position),radius_(radius){}

    /**
     * \brief Деструктор
     */
    ~Sphere() override = default;

    /**
     * \brief Пересечение сферы и луча
     * \param ray Луч
     * \param tMin Минимальное расстояние
     * \param tMax Максимальное расстояние
     * \param tOut Итоговое расстояние до точки пересечения
     * \param normalOut Нормаль поверхности в точке пересечени
     * \return Было ли пересечение с объектом
     */
    bool intersectsRay(const math::Ray& ray, float tMin, float tMax, HitInfo* hitInfo) const override
    {
        // Дистанция до пересечения
        float t = 0;

        // Если пересеченеи было
        if(ray.intersectsSphere(position_,radius_,tMin,tMax,&t))
        {
            // Если указатель на структуру информации о пересечении был передан
            if(hitInfo != nullptr)
            {
                // Запись значений
                hitInfo->t = t;
                hitInfo->point = ray.getOrigin() + (ray.getDirection() * t);
                hitInfo->normal = math::Normalize(hitInfo->point - position_);
                hitInfo->frontFaceSurface = true;

                // Если нормаль не направлена против луча, считать что это обратная сторона (и инвертировать нормаль)
                if(math::Dot(-ray.getDirection(),hitInfo->normal) < 0.0f){
                    hitInfo->normal = -hitInfo->normal;
                    hitInfo->frontFaceSurface = false;
                }
            }
            return true;
        }
        return false;
    }
};