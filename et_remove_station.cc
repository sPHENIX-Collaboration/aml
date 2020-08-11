/* little utility to get rid of orphaned stations of an et pool */

#include <stdio.h>
#include "et.h"


int main(int argc, char **argv)
{
  et_sys_id	et_id;
  et_openconfig et_config;
  int num_stations;
  int iret, status;
  int nattach;
  et_stat_id stat_id;

  if ((argc != 2 && argc != 3))
    {
      printf("Usage: et_remove_station <et_pool> <station_name>\n");
      printf("       if only <et_pool> is given all unused stations are removed\n");
      exit(1);
    }

  /* input et pool */
  et_open_config_init(&et_config);
  et_open_config_sethost(et_config, ET_HOST_ANYWHERE);

  if ( (iret = et_open(&et_id, argv[1], et_config)) != ET_OK)
    {
      printf("et_remove_station: cannot open ET system iret: %d\n", iret);
      exit(1);
    }
  et_open_config_destroy(et_config);

  if ((status = et_system_getstationsmax(et_id, &num_stations)) != ET_OK)
    {
      printf("ERROR in et_system_getstations: status %d\n", status);
      exit(1);
    }

  printf("number of stations in et pool %s: %d\n", argv[1], num_stations);
  if (argc == 2)
    {
      /* start at 1 to leave GRAND CENTRAL out - can't remove it anyway */
      for (stat_id = 1; stat_id < num_stations;stat_id++)
        {
          et_station_getstatus(et_id, stat_id, &status);
          if (status)
            {
              if (et_station_getattachments(et_id, stat_id, &nattach) == ET_OK)
                {
                  if (!nattach)
                    {
                      printf("Removing station %d\n", stat_id);
                      et_station_remove(et_id, stat_id);
                    }
		  else
		    {
                      printf("station %d is in use\n", stat_id);
		    }
                }
            }
        }

    }
  else
    {
      if (et_station_exists(et_id, &stat_id, argv[2]))
        {
          printf("checking attachments to station %s\n", argv[2]);
          printf("Station id: %d\n", stat_id);
          et_station_getattachments(et_id, stat_id, &nattach);
          if (!nattach)
            {
              printf("no attachments to station %s, removing it\n", argv[2]);
              et_station_remove(et_id, stat_id);
            }
          else
            {
              printf("station %s has %d attachment(s), leaving it\n", argv[2], nattach);
            }
        }
      else
        {
          printf("Station %s does not exist\n", argv[2]);
        }
    }
}



