#include "image_extract.h"

unsigned int		get_slices_max(t_vol *volume)
{
  // get the larger number of slices possible
  if ((volume->size[X] * volume->size[Y]) > (volume->size[Z] * volume->size[X]))
    {
      if ((volume->size[X] * volume->size[Y]) > (volume->size[Z] * volume->size[Y]))
	return (volume->size[X] * volume->size[Y]);
    }
  else if ((volume->size[Z] * volume->size[X]) > (volume->size[Z] * volume->size[Y]))
    return (volume->size[Z] * volume->size[X]);
  return (volume->size[Z] * volume->size[Y]);
}

void            *get_all_slices_of_all_dimensions(void *args)
{
  t_vol		*volume;
  t_image_args	*a;
  int           i;
  unsigned long *start;
  long unsigned int *count;

  a = (t_image_args *)args;
  volume = a->volume;
  i = 0;
  // init start and count variable
  count = malloc(volume->dim_nb * sizeof(*count));
  while (i < volume->dim_nb)
    {
      count[i] = volume->size[i];
      i++;
    }
  start = malloc(volume->dim_nb * sizeof(*start));
  start[X] = start[Y] = start[Z] = 0; // start to 0 = first slice
  // loop all dimensions
  i = 0;
  while (i < volume->dim_nb)
    {
      // check if the dimension need to be ignored
      if (a->dim_start_end[i][0] != -1 &&
	  a->dim_start_end[i][1] != -1)
	{
	  // set count variable to 1 for the current dimension to indicate the step (here 1 == 1 slice per time)
	  count[i] = 1;
	  get_all_slices_of_one_dimension(volume, start, i, count, a);
	  count[i] = volume->size[i];
	}
      i++;
    }
  // some free variable
  free_image_args(a);
  if (start != NULL) free(start);
  if (count != NULL) free(count);
  return (NULL);
}

void		get_raw_data_hyperslab(t_vol *volume, int dim, int slice, char *hyperslab)
{
  unsigned long long int offset;

  if (volume->raw_data == 1)
    {
      offset = (volume->dim_offset[dim] + (unsigned long long int)((unsigned long long int)volume->slice_size[dim] * (unsigned long long int)slice));
      /*#ifdef __off64_t_defined
      //#ifndef __USE_LARGEFILE64
      printf("hello\n");*/
      lseek(volume->raw_fd, offset, SEEK_SET);
      /*#else
      unsigned long long int count;
      unsigned long long int modulo;
      unsigned long int i;

      printf("hello1\n");
      i = 0;
      count = offset / 100000000;
      modulo = offset % 100000000;
      lseek(volume->raw_fd, 0, SEEK_SET);
      while (i < count)
	{
	  lseek(volume->raw_fd, 100000000 - 1, SEEK_CUR);
	  i++;
	}
      lseek(volume->raw_fd, modulo, SEEK_CUR);
      #endif*/
      read(volume->raw_fd, hyperslab, volume->slice_size[dim]);
    }
}

void            get_all_slices_of_one_dimension(t_vol *volume, unsigned long *start, int current_dimension,
						long unsigned int *count, t_image_args *a)
{
  unsigned int	current_slice;
  unsigned int	max;
  int		width;
  int		height;
  char	        *hyperslab;
  int		w_max_iteration;
  int		h_max_iteration;
  int		save_h_position = a->info->h_position;
  int		save_w_position = a->info->w_position;

  // allocation of a hyperslab (portion of the file, can be 1 slice or 1 demension...)
  hyperslab =  malloc(volume->slices_max * sizeof(*hyperslab));
  // set the first slice extracted
  current_slice = (unsigned int)a->dim_start_end[current_dimension][0];
  // set the last slice extracted
  max = (unsigned int)(a->dim_start_end[current_dimension][1] == 0 ?
		       volume->size[current_dimension] : a->dim_start_end[current_dimension][1]);
  // init height and width compare to the dimension
  get_width_height(&height, &width, current_dimension, volume);
  // loop all the slices
  while (current_slice < max)
    {
      memset(hyperslab, 0, (volume->slices_max * sizeof(*hyperslab)));
      start[current_dimension] = current_slice;
      // get the data of 1 slice
      if (volume->raw_data != 1)
	{
	  pthread_mutex_lock(&(a->p->lock));
	  miget_real_value_hyperslab(volume->minc_volume, MI_TYPE_UBYTE, start, count, hyperslab);
	  pthread_mutex_unlock(&(a->p->lock));
	}
      else
	get_raw_data_hyperslab(volume, current_dimension, current_slice, hyperslab);
      // print image
      if (a->info->h_position == -1 && a->info->w_position == -1)
	{
	  a->info->h_position = 0;
	  a->info->start_h = 0;
	  a->info->w_position = 0;
	  a->info->start_w = 0;

	  h_max_iteration = (height * a->info->scale) / a->info->square_size;
	  w_max_iteration = (width * a->info->scale) / a->info->square_size;

	  while (a->info->start_h <= h_max_iteration)
	    {
	      a->info->start_w = 0;
	      while (a->info->start_w <= w_max_iteration)
		{
		  a->info->h_position = a->info->start_h;
		  a->info->w_position = a->info->start_w;

		  print_image(hyperslab, volume, current_dimension, current_slice, width, height, a);
		  a->info->start_w++;
		}
	      a->info->start_h++;
	    }
	}
      else {
	print_image(hyperslab, volume, current_dimension, current_slice, width, height, a);
      }

      pthread_mutex_lock(&(a->p->lock));
      a->info->slices_done++;
      pthread_mutex_unlock(&(a->p->lock));
      pthread_cond_signal(&(a->info->cond));
      current_slice++;

      // restore original positions
      a->info->h_position = save_h_position;
      a->info->w_position = save_w_position;
    }
  start[current_dimension] = 0;
  free(hyperslab);
}
