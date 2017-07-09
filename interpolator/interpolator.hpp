#pragma once

#include <map>

template <typename X, typename Y>
class interpolator
{
private:
    using points_map_t = std::map<X, Y>;

public:
    using point_t = std::pair<typename points_map_t::key_type, typename points_map_t::mapped_type>;
    using result_map_t = points_map_t;

    interpolator() {}
    ~interpolator() {}

    bool add(const X & x, const Y & y)
    {
        m_points.emplace(x, y);

        bool valid = isValid(y);

        if (valid)
            m_validPoints[x] = y;
        else
            m_validPoints.erase(x);

        return valid;
    }

    point_t get(const X & x)
    {
        point_t result {x, invalidValue()};

        auto i = m_validPoints.find(x), vp_end = std::end(m_validPoints);

        if (i == vp_end)
        {
            // interpolate
            auto vp_right = m_validPoints.lower_bound(x), vp_left = vp_right;

            if (vp_right != vp_end && vp_right != std::begin(m_validPoints))
            {
                --vp_left;
            }

            if (vp_left != vp_end && vp_right != vp_end)
            {
                result.second = interpolate(x, *vp_left, *vp_right);
            }
        }
        else
        {
            result.second = i->second;
        }

        return result;
    }

    void update(const X & x, const Y & y, result_map_t & result)
    {
        // insert point
        m_points[x] = y;

        bool valid = isValid(y);

        typename points_map_t::iterator vp_left, vp_right, vp_mid;

        if (valid)
        {
            vp_mid = m_validPoints.find(x);

            if (vp_mid == m_validPoints.end())
                vp_mid = m_validPoints.emplace(x, y).first;
            else
                *vp_mid = y;

            vp_left = vp_right = vp_mid;

            if (vp_left != std::begin(m_validPoints))
                --vp_left;

            if (++vp_right == std::end(m_validPoints))
                vp_right = vp_mid;
        }
        else
        {
            m_validPoints.erase(x);

            vp_left = m_validPoints.lower_bound(x);
            vp_right = std::next(vp_left);

            auto vp_end = std::end(m_validPoints);

            if (vp_left != vp_end && vp_right != vp_end)
            {
                for (auto pt = m_points.find(vp_left->first), pte = m_points.find(vp_right->first); pt != pte; ++pt)
                {
                    result.emplace(std::move(interpolate(pt->first, *vp_left, *vp_right)));
                }
            }
        }

        // to compute affected range
        // * calculate valid point from left and right (VP_lhs & VP_rhs)
        // cases:
        // 1. the new point P is valid
        //     affected range: [VP_lhs;P] - [P; VP_rhs]
        // 2. the new point is invalid
        //     affected range: [VP_lhs; VP_rhs]
    }

    bool empty() const noexcept
    {
        return m_points.empty();
    }

    std::size_t size() const noexcept
    {
        return m_points.size();
    }

    std::size_t valid_count() const noexcept
    {
        return m_validPoints.size();
    }

    // overrides
    // TODO: should we use traits?
    virtual bool isValid(const Y & y) const = 0;
    virtual Y invalidValue() const = 0;

    virtual Y interpolate(const X & x, const point_t & p0, const point_t & p1) const
    {
        if (p0.first == p1.first)
            return invalidValue();

        return p0.second + (p1.second - p0.second) * (x - p0.first) / (p1.first - p0.first);
    }

private:
    points_map_t m_points, m_validPoints;
};
