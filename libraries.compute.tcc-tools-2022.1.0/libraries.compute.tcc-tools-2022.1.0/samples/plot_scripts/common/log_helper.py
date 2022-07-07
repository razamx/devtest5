# /*******************************************************************************
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/


import numpy as np
import matplotlib.pyplot as plt
import os


NBINS = 100             # Number of bins for histogram


# Calcs and print measurements' statistics
def analyze_data(data, name, time_unit):
    dmin = np.min(data)
    dmax = np.max(data)

    print('---- %s ----' % name)
    print('min:    %f %s' % (dmin, time_unit))
    print('avg:    %f %s' % (np.average(data), time_unit))
    print('max:    %f %s' % (dmax, time_unit))
    print('std:    %f %s' % (np.std(data), time_unit))
    print('jitter: %f %s' % ((dmax - dmin, time_unit)))

# converts data in seconds to the time unit
def convert_time_unit(data, time_unit):
    if time_unit == 's':
        return data
    elif time_unit == 'ms':
        return data*1000
    elif time_unit == 'us':
        return data*1000000
    elif time_unit == 'ns':
        return data*1000000000

# Check file exists and exit
def check_file(filename):
    if not os.path.exists(filename):
        print('File %s not found' % filename)
        exit(1)

def __calc_bins(dbounds, bounds):
    dmin, dmax = dbounds
    bmin, bmax = bounds
    bin_bounds = dmin if bmin is None else bmin, dmax if bmax is None else bmax

    if bin_bounds[1] < dbounds[0]:
        print('--max is lower than minimum data value ({})'.format(dbounds[0]))
        print('Cannot plot the data')
        return None
    if bin_bounds[0] > dbounds[1]:
        print('--min is bigger than maximum data value ({})'.format(dbounds[1]))
        print('Cannot plot the data')
        return None

    return np.linspace(*bin_bounds, NBINS)

# Plots data
def plot_scatter(data, names, title, fig, grid, units='s', grid_offset=0, bounds=(None, None), merge=True):
    """
    Plots data:
      - scatter plot, every sample represented as a dot
      - histogram plot for distribution
    Args:
      - data        data, one or more measurements
      - names       measurements names, same count as data
      - title       common plot title
      - fig         fig to create subplots
      - grid        grid to create sublots
      - units       time units
      - grid_offset
      - bounds      bounds for histogram
      - merge       draw two measurements in one plot or in two separates subplots, one under the other
      Return:
        Is picture was plotted
    """
    if not len(data):
        return

    # Create layout, subplots, set axis settings etc
    scatters = []
    hists = []
    alpha = 0.6 if merge else 1
    for i in range(1) if merge else range(len(data)):
        if i == 0:
            s = fig.add_subplot(grid[grid_offset + i,1:])
            h = fig.add_subplot(grid[grid_offset + i,0], sharey=s)
        else:
            s = fig.add_subplot(grid[grid_offset + i,1:], sharex=scatters[i-1], sharey=scatters[i-1])
            h = fig.add_subplot(grid[grid_offset + i,0], sharex=hists[i-1], sharey=s)
            h.invert_xaxis()

        # Hide Y axis labels overlapped by Histogram
        plt.setp(s.get_yticklabels(), visible=False)

        h.set_ylabel('Time, %s' % units)
        h.set_xscale('log')
        h.ticklabel_format(style='plain', axis='y')
        h.invert_xaxis()

        # Hide X axis minor ticks
        h.xaxis.set_minor_locator(plt.NullLocator())
        # Space ticks logarithmically in base of 10
        h.xaxis.set_major_locator(plt.LogLocator(base=10))

        scatters.append(s)
        hists.append(h)

    scatters[-1].set_xlabel('Iteration')
    hists[-1].set_xlabel('Count')

    # Create bins for histogram
    min = np.min([np.min(row) for row in data])
    max = np.max([np.max(row) for row in data])
    bins = __calc_bins((min, max), bounds)

    # Draw data
    if bins is not None:
        for i, d in enumerate(data):
            index = 0 if merge else i
            scatters[index].plot(d,  'o', alpha=0.6, markersize=1, label=names[i])
            hists[index].hist(d, bins, orientation='horizontal', alpha=alpha)

        # Setup user-defined axis limits
        # Note: limits must be configured after plotting in order to support setup
        # only one (lower or upper) limit while other is selected automatically
        for hist, scatter in zip(hists, scatters):
            hist.set_ylim(bounds)
            scatter.set_ylim(bounds)

        # Draw legend (measurement names)
        for scatter in scatters:
            scatter.legend()

        scatters[0].set_title(title)

    # Print statistics
    print('Statistics:')
    for i, d in enumerate(data):
        analyze_data(d, names[i], units)

    return bins is not None
