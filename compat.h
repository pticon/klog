#ifndef __COMPAT_H__

#define __COMPAT_H__


#ifdef __FreeBSD__
# include <dev/evdev/input.h>
#else
# include <linux/input.h>
#endif /* __FreeBSD__ */

#endif /* __COMPAT_H__ */
