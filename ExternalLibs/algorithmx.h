#ifndef STDXfg_ALGORITHMX_H
#define STDXfg_ALGORITHMX_H

// Some std:: namespace enhancements

#include <algorithm>

namespace stdx_fg {

template <class ForwardIterator, class T, class Compare>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& val, Compare comp)
{
    ForwardIterator it;
    typename std::iterator_traits<ForwardIterator>::difference_type count, step;
    count = std::distance(first,last);
    while(count > 0)
    {
        it = first;
        step = count/2;
        std::advance (it,step);
        if (comp(*it, val))
        {
            first= ++it;
            count -= step+1;
        }
        else
            count = step;
    }
    return first;
}

} // end namespace stdx_fg

#endif
