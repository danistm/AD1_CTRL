/*
 * Model::tick() runs once per GUI frame on EVERY screen (splash, main,
 * menu), which is exactly what the XMOS link and the rate arbiter need:
 * packets received while the menu is open are decoded immediately, and
 * a host rate change is applied no matter what is on the glass.
 * (gui/ is user-owned: Designer never overwrites this file.)
 */
#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "xmos_link.h"       /* Core/Inc is on the gui include path     */
#include "rate_ctrl.h"

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
	Xmos_Service();          /* decode packets, RDY bookkeeping, watchdog */
	RateCtl_Service();       /* apply host rate / revert on host gone     */
}
