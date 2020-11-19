#pragma once

#include "../Utils.h"

namespace materials
{
    class Refractive : public Material
    {
    private:
        /// Коэффициент преломления
        float refractionIndex_;

        /**
         * \brief Подсчет степени отражения (Аппроксимация Шлика)
         * \param v Вектор падения луча
         * \param normal Нормаль поверхности
         * \param refractionRatio Обратный коээфициент преломления
         * \return Степень отражения
         */
        static float reflectance(const math::Vec3<float>& v, const math::Vec3<float>& normal, const float& refractionRatio)
        {
            // Использовать аппроксимациб Шлика для вычисления отражательной способности по углу падения и коэффициенту преломления
            float cosine = std::max(math::Dot(-v,normal),0.0f);
            auto r0 = (1.0f - refractionRatio) / (1.0f + refractionRatio);
            r0 = r0 * r0;
            return r0 + (1.0f - r0) * powf((1.0f - cosine),5);
        }

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Refractive(): Material(), refractionIndex_(1.0f){}

        /**
         * \brief Основной конструктор
         * \param refractionIndex Индекс преломления
         */
        explicit Refractive(float refractionIndex): Material(), refractionIndex_(refractionIndex){}

        /**
         * \brief Происходит ли переотражение-разброс лучей для данного материала
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \return Да или нет
         */
        bool isScatters(const math::Ray& rayIn, const HitInfo& hitInfo) const override
        {
            // Данные о входном луче и пересечении не задействованы
            (void) rayIn;
            (void) hitInfo;

            // Материалу свойственен разброс лучей
            return true;
        }

        /**
         * \brief Излучает ли данный материал свет
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \return Да или нет
         */
        bool isEmits(const math::Ray& rayIn, const HitInfo& hitInfo) const override
        {
            // Данные о входном луче и пересечении не задействованы
            (void) rayIn;
            (void) hitInfo;

            // Материал не излучает
            return false;
        }

        /**
         * \brief Получить переотраженный-разбросанный луч
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \param attenuationOut Затухание для переотраженного луча
         * \return Переотраженный луч
         */
        math::Ray scatteredRay(const math::Ray& rayIn, const HitInfo& hitInfo, math::Vec3<float>* attenuationOut) const override
        {
            // Коэффициент преломления инвертируется если луч выходит из вещества (удар о нелицевую сторону объекта)
            float refractionIndex = hitInfo.frontFaceSurface ? refractionIndex_ : 1.0f / refractionIndex_;

            // По умолчанию - луч отражается
            math::Vec3<float> scatteredDir = math::Reflect(rayIn.getDirection(), hitInfo.normal);

            // Если отражательной способности не достаточно - луч преломляется
            // Граница отражательной способности - случайное значение для каждого луча
            // Таким образом объект частично отражает, частично преломляет
            if(reflectance(rayIn.getDirection(),hitInfo.normal,1.0f/refractionIndex) < RndFloat()){
                scatteredDir = math::Refract(rayIn.getDirection(),hitInfo.normal,refractionIndex,true);
            }

            // Отраженный от точки пересечения луч распространяется в случайном направлении в пределах полсуферы
            math::Ray scattered(hitInfo.point,scatteredDir);

            // Нет затухания (свет преломленного луча передается как есть)
            if(attenuationOut){
                *attenuationOut = {1.0f,1.0f,1.0f};
            }

            return scattered;
        }

        /**
         * \brief Излученный цвет
         * \return Цветовой вектор
         */
        math::Vec3<float> emittedColor() const override
        {
            return {0.0f,0.0f,0.0f};
        }
    };
}