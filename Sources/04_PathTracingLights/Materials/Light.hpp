#pragma once

#include "../Utils.h"

namespace materials
{
    class Light : public Material
    {
    private:
        /// Цвет излучаемого света
        math::Vec3<float> color_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Light():Material(),color_({1.0,1.0f,1.0f}){}

        /**
         * \brief Основной конструктор
         * \param albedo Цвет излучаемого света
         */
        explicit Light(const math::Vec3<float>& albedo = {1.0f,1.0f,1.0f}):Material(),color_(albedo){}

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

            // Материалу не свойственен разброс лучей
            return false;
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

            // Материал излучает свет только с лицевой стороны
            return hitInfo.frontFaceSurface;
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
            // Данные о входном луче, пересечении а также указатель на затухание не задействованы
            (void) rayIn;
            (void) hitInfo;
            (void) attenuationOut;

            // Вернуть пучтой объект
            return {};
        }

        /**
         * \brief Излученный цвет
         * \return Цветовой вектор
         */
        math::Vec3<float> emittedColor() const override
        {
            return color_;
        }
    };
}