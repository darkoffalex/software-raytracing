#pragma once

#include "../Utils.h"

namespace scene
{
    class Sphere : public Hittable
    {
    private:
        /// Положение центра сферы в пространстве
        math::Vec3<float> position_;
        /// Радиус сферы
        float radius_;
        /// Инвертировать нормали
        bool flipNormals_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Sphere():
                Hittable(), position_({0.0f,0.0f,0.0f}), radius_(1.0f), flipNormals_(false){};

        /**
         * \brief Основной конструктор
         * \param materialPtr Указатель на материал сферы
         * \param position Положение центра сферы
         * \param radius Радиус сферы
         * \param inverted Вывернутая сфера
         */
        Sphere(const std::shared_ptr<materials::Material>& materialPtr, const math::Vec3<float>& position, const float& radius, bool inverted = false):
                Hittable(materialPtr), position_(position), radius_(radius), flipNormals_(inverted){}

        /**
         * \brief Деструктор
         */
        ~Sphere() override = default;

        /**
         * \brief Пересечение луча и сферы
         * \param ray Луч
         * \param tMin Минимальное расстояние
         * \param tMax Максимальное расстояние
         * \param hitInfo Информация о пересечении
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
                    hitInfo->materialPtr = this->materialPtr_;

                    // Если нужно инвертировать нормали
                    if(flipNormals_){
                        hitInfo->normal = -hitInfo->normal;
                    }

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
}