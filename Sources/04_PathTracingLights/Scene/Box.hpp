#pragma once

#include "../Utils.h"

namespace scene
{
    class Box : public Hittable
    {
    private:
        /// Положение центральной точки (локальный центр) ящика
        math::Vec3<float> position_;
        /// Ориентация в пространстве
        math::Vec3<float> orientation_;
        /// Размеры ящика
        math::Vec3<float> sizes_;
        /// Инвертировать нормали
        bool flipNormals_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Box():
        Hittable(),position_({0.0f,0.0f,0.0f}),orientation_({0.0f,0.0f,0.0f}),sizes_({1.0f,1.0f,1.0f}),flipNormals_(false){}

        /**
         * \brief Основной конструктор
         * \param materialPtr Материал
         * \param position Положение центра ящика
         * \param sizes Размеры ящика
         * \param orientation Ориентация в пространстве
         * \param flipped Инвертировать нормали
         */
        Box(
          const std::shared_ptr<materials::Material>& materialPtr,
          const math::Vec3<float>& position,
          const math::Vec3<float>& sizes = {1.0f,1.0f,1.0f},
          const math::Vec3<float>& orientation = {0.0f,0.0f,0.0f},
          bool flipped = false):
        Hittable(materialPtr),position_(position),orientation_(orientation),sizes_(sizes),flipNormals_(flipped){}

        /**
         * \brief Деструктор
         */
        ~Box() override = default;

        /**
         * \brief Пересечение луча и прямоугольника
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

            // Для того чтобы трансформировать объект (положение, ориентацию) нужно применить обратную трансформацию к лучу
            // T.е сдвигается не сам объект, а луч относительно объекта.
            auto inverseRotation = math::GetRotationMat(-orientation_);

            // Траснформированный луч
            math::Ray transformedRay(
                    inverseRotation * (ray.getOrigin() - position_),
                    inverseRotation * ray.getDirection());

            // Половинные ширина, высота и длина
            float halfWidth = this->sizes_.x / 2.0f;
            float halfHeight = this->sizes_.y / 2.0f;
            float halfLength = this->sizes_.z / 2.0f;

            // Расстояние до ближ. пересечения
            float tClosest = tMax;
            // Было ли пересечение с каким-либо объектом
            bool hitAnything = false;
            // Нормаль в точке ближ. пересечения
            math::Vec3<float> normalClosest = {0.0f,0.0f,0.0f};

            // Проход по граням ящика
            for(unsigned i = 0; i < 6; i++)
            {
                switch (i)
                {
                    default:
                    case 0:
                        if(transformedRay.intersectsAARectangleXy(-halfLength,-halfWidth,halfWidth,-halfHeight,halfLength,tMin,tClosest,&t)){
                            hitAnything = true;
                            tClosest = t;
                            normalClosest = {0.0f,0.0f,-1.0f};
                        }
                        break;
                    case 1:
                        if(transformedRay.intersectsAARectangleXy(halfLength,-halfWidth,halfWidth,-halfHeight,halfLength,tMin,tClosest,&t)){
                            hitAnything = true;
                            tClosest = t;
                            normalClosest = {0.0f,0.0f,1.0f};
                        }
                        break;
                    case 2:
                        if(transformedRay.intersectsAARectangleYz(-halfWidth,-halfHeight,halfHeight,-halfLength,halfLength,tMin,tClosest,&t)){
                            hitAnything = true;
                            tClosest = t;
                            normalClosest = {-1.0f,0.0f,0.0f};
                        }
                        break;
                    case 3:
                        if(transformedRay.intersectsAARectangleYz(halfWidth,-halfHeight,halfHeight,-halfLength,halfLength,tMin,tClosest,&t)){
                            hitAnything = true;
                            tClosest = t;
                            normalClosest = {1.0f,0.0f,0.0f};
                        }
                        break;
                    case 4:
                        if(transformedRay.intersectsAARectangleXz(-halfHeight,-halfWidth,halfWidth,-halfLength,halfLength,tMin,tClosest,&t)){
                            hitAnything = true;
                            tClosest = t;
                            normalClosest = {0.0f,-1.0f,0.0f};
                        }
                        break;
                    case 5:
                        if(transformedRay.intersectsAARectangleXz(halfHeight,-halfWidth,halfWidth,-halfLength,halfLength,tMin,tClosest,&t)){
                            hitAnything = true;
                            tClosest = t;
                            normalClosest = {0.0f,1.0f,0.0f};
                        }
                        break;
                }
            }

            // Если было какое-то пересечение и указатель на структуру информации о пересечении был передан
            if(hitAnything && hitInfo != nullptr)
            {
                // Запись значений
                hitInfo->t = t;
                hitInfo->point = ray.getOrigin() + (ray.getDirection() * t);
                hitInfo->normal = normalClosest;
                hitInfo->frontFaceSurface = true;
                hitInfo->materialPtr = this->materialPtr_;

                // Если нужно инвертировать нормали
                if(flipNormals_){
                    hitInfo->normal = -hitInfo->normal;
                }

                // Если нормаль не направлена против луча, считать что это обратная сторона (и инвертировать нормаль)
                if(math::Dot(-transformedRay.getDirection(),hitInfo->normal) < 0.0f){
                    hitInfo->normal = -hitInfo->normal;
                    hitInfo->frontFaceSurface = false;
                }

                // Поскольку нормаль считалась в пространстве объекта ее нужно перевести в глобальное пространство
                hitInfo->normal = math::GetRotationMat(orientation_) * hitInfo->normal;
            }

            return hitAnything;
        }
    };
}