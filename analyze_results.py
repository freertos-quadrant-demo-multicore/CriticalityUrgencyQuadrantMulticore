#!/usr/bin/env python3
"""
Analyze and visualize Multi-Core SMP test results
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
from datetime import datetime

# Data from test runs
results = {
    'Dedication': {
        'total_utilization': 1.846,
        'core_utilizations': [0.660, 0.270, 0.821, 0.095],
        'core_tasks': [4, 4, 4, 4],
        'core_executions': [905, 92, 1097, 10],
        'core_misses': [0, 0, 0, 0],
        'total_executions': 1267,
        'total_misses': 0,
        'q1_misses': 0,
        'q2_misses': 0,
        'q3_misses': 0,
        'q4_misses': 0,
        'lbi': 0.116,
        'avg_utilization': 0.462,
        'task_allocation': {
            'Core 0': ['CAN_Handler', 'Brake_Monitor', 'Sensor_Fusion', 'Emergency_Shutdown'],
            'Core 1': ['Diagnostics', 'Watchdog_Refresh', 'NVM_Logging', 'Health_Monitor'],
            'Core 2': ['HMI_Render', 'Touch_Handler', 'Audio_Buffer', 'Nav_Update'],
            'Core 3': ['Telemetry', 'Data_Logging', 'File_Cleanup', 'Statistics']
        }
    },
    'PU-WFD': {
        'total_utilization': 1.846,
        'core_utilizations': [0.310, 0.350, 0.520, 0.666],
        'core_tasks': [2, 2, 6, 6],
        'core_executions': [352, 552, 643, 556],
        'core_misses': [0, 0, 0, 0],
        'total_executions': 1266,
        'total_misses': 0,
        'q1_misses': 0,
        'q2_misses': 0,
        'q3_misses': 0,
        'q4_misses': 0,
        'lbi': 0.466,
        'avg_utilization': 0.462,
        'task_allocation': {
            'Core 0': ['Sensor_Fusion', 'Brake_Monitor'],
            'Core 1': ['CAN_Handler', 'Emergency_Shutdown'],
            'Core 2': ['Diagnostics', 'NVM_Logging', 'Health_Monitor', 'Watchdog_Refresh', 'Nav_Update', 'Audio_Buffer'],
            'Core 3': ['HMI_Render', 'Touch_Handler', 'Data_Logging', 'Statistics', 'File_Cleanup', 'Telemetry']
        }
    }
}

def create_comparison_tables():
    """Generate comparison tables in Markdown format"""

    with open('SMP_RESULTS_TABLES.md', 'w') as f:
        f.write("# Multi-Core SMP Experimental Results\n\n")
        f.write(f"**Test Date**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        f.write("**Configuration**: 4-core system, 16 automotive tasks\n\n")

        # Table 1: Overall Performance Comparison
        f.write("## Table 1: Overall Performance Comparison\n\n")
        f.write("| Metric | Dedication | PU-WFD | Difference |\n")
        f.write("|--------|-----------|--------|------------|\n")

        ded = results['Dedication']
        wfd = results['PU-WFD']

        f.write(f"| Average Core Utilization | {ded['avg_utilization']:.3f} ({ded['avg_utilization']*100:.1f}%) | {wfd['avg_utilization']:.3f} ({wfd['avg_utilization']*100:.1f}%) | {abs(ded['avg_utilization']-wfd['avg_utilization'])*100:.1f}% |\n")
        f.write(f"| Load Balance Index (LBI) | {ded['lbi']:.3f} | {wfd['lbi']:.3f} | +{(wfd['lbi']-ded['lbi'])*100:.1f}% |\n")
        f.write(f"| Total Executions (5 sec) | {ded['total_executions']} | {wfd['total_executions']} | {wfd['total_executions']-ded['total_executions']} |\n")
        f.write(f"| Total Deadline Misses | {ded['total_misses']} ✓ | {wfd['total_misses']} ✓ | {wfd['total_misses']-ded['total_misses']} |\n")
        f.write(f"| Q1 (Critical) Misses | {ded['q1_misses']} ✓ | {wfd['q1_misses']} ✓ | {wfd['q1_misses']-ded['q1_misses']} |\n")

        max_util_ded = max(ded['core_utilizations'])
        max_util_wfd = max(wfd['core_utilizations'])
        min_util_ded = min(ded['core_utilizations'])
        min_util_wfd = min(wfd['core_utilizations'])

        f.write(f"| Max Core Utilization | {max_util_ded:.3f} ({max_util_ded*100:.1f}%) | {max_util_wfd:.3f} ({max_util_wfd*100:.1f}%) | {abs(max_util_ded-max_util_wfd)*100:.1f}% |\n")
        f.write(f"| Min Core Utilization | {min_util_ded:.3f} ({min_util_ded*100:.1f}%) | {min_util_wfd:.3f} ({min_util_wfd*100:.1f}%) | {abs(min_util_ded-min_util_wfd)*100:.1f}% |\n")

        # Table 2: Per-Core Statistics
        f.write("\n## Table 2: Per-Core Utilization - Dedication Strategy\n\n")
        f.write("| Core | Primary Quadrant | Utilization | Tasks | Executions | Misses |\n")
        f.write("|------|-----------------|-------------|-------|------------|--------|\n")

        primaries_ded = ['Q1', 'Q1', 'Q2', 'Q3']
        for i in range(4):
            f.write(f"| {i} | {primaries_ded[i]} | {ded['core_utilizations'][i]:.3f} ({ded['core_utilizations'][i]*100:.1f}%) | {ded['core_tasks'][i]} | {ded['core_executions'][i]} | {ded['core_misses'][i]} |\n")

        f.write("\n## Table 3: Per-Core Utilization - PU-WFD Strategy\n\n")
        f.write("| Core | Primary Quadrant | Utilization | Tasks | Executions | Misses |\n")
        f.write("|------|-----------------|-------------|-------|------------|--------|\n")

        primaries_wfd = ['Q1', 'Q1', 'Q2', 'Q3']
        for i in range(4):
            f.write(f"| {i} | {primaries_wfd[i]} | {wfd['core_utilizations'][i]:.3f} ({wfd['core_utilizations'][i]*100:.1f}%) | {wfd['core_tasks'][i]} | {wfd['core_executions'][i]} | {wfd['core_misses'][i]} |\n")

        # Table 4: Task Allocation Comparison
        f.write("\n## Table 4: Task Allocation Comparison\n\n")
        f.write("| Core | Dedication Strategy | PU-WFD Strategy |\n")
        f.write("|------|-------------------|----------------|\n")

        for i in range(4):
            core_key = f'Core {i}'
            ded_tasks = ', '.join(ded['task_allocation'][core_key])
            wfd_tasks = ', '.join(wfd['task_allocation'][core_key])
            f.write(f"| {i} | {ded_tasks} | {wfd_tasks} |\n")

        # Key Findings
        f.write("\n## Key Findings\n\n")
        f.write("### Safety Properties (MAINTAINED) ✓\n")
        f.write("- **Q1 Deadline Miss Rate**: 0.00% for both strategies\n")
        f.write("- **Q2 Deadline Miss Rate**: 0.00% for both strategies\n")
        f.write("- **Q3 Deadline Miss Rate**: 0.00% for both strategies\n")
        f.write("- **Q4 Deadline Miss Rate**: 0.00% for both strategies\n\n")

        lbi_improvement = ((wfd['lbi'] - ded['lbi']) / ded['lbi']) * 100
        f.write("### Load Balancing\n")
        f.write(f"- **Dedication LBI**: {ded['lbi']:.3f} (Poor - uneven distribution)\n")
        f.write(f"- **PU-WFD LBI**: {wfd['lbi']:.3f} (Poor but {lbi_improvement:.1f}% better)\n")
        f.write(f"- **Improvement**: PU-WFD achieves {lbi_improvement:.1f}% better load balance\n\n")

        f.write("### Utilization Distribution\n")
        f.write(f"- **Dedication**: Max={max_util_ded*100:.1f}%, Min={min_util_ded*100:.1f}%, Range={( max_util_ded-min_util_ded)*100:.1f}%\n")
        f.write(f"- **PU-WFD**: Max={max_util_wfd*100:.1f}%, Min={min_util_wfd*100:.1f}%, Range={(max_util_wfd-min_util_wfd)*100:.1f}%\n")
        f.write(f"- **Analysis**: PU-WFD reduces utilization range by {((max_util_ded-min_util_ded)-(max_util_wfd-min_util_wfd))*100:.1f}%\n\n")

        f.write("### Task Distribution\n")
        f.write(f"- **Dedication**: Fixed 4 tasks per core (rigid allocation)\n")
        f.write(f"- **PU-WFD**: Variable tasks per core (2-6 tasks, adaptive)\n\n")

        f.write("### Observations\n")
        f.write("1. **Both strategies maintain 100% safety** (0 deadline misses for all quadrants)\n")
        f.write("2. **PU-WFD improves load balance** by spreading load more evenly\n")
        f.write("3. **Average utilization is identical** (~46.2%) showing same total work\n")
        f.write("4. **Dedication has extreme imbalance**: Core 2 at 82.1%, Core 3 at 9.5%\n")
        f.write("5. **PU-WFD reduces imbalance**: Max 66.6%, Min 31.0%\n")

    print("✓ Created SMP_RESULTS_TABLES.md")

def create_visualizations():
    """Generate graphs and visualizations"""

    # plt.style.use('default')  # Use default style
    fig = plt.figure(figsize=(16, 12))

    # Graph 1: Core Utilization Comparison
    ax1 = plt.subplot(2, 3, 1)
    cores = ['Core 0', 'Core 1', 'Core 2', 'Core 3']
    x = np.arange(len(cores))
    width = 0.35

    ded_utils = [u * 100 for u in results['Dedication']['core_utilizations']]
    wfd_utils = [u * 100 for u in results['PU-WFD']['core_utilizations']]

    bars1 = ax1.bar(x - width/2, ded_utils, width, label='Dedication', color='#3498db', alpha=0.8)
    bars2 = ax1.bar(x + width/2, wfd_utils, width, label='PU-WFD', color='#e74c3c', alpha=0.8)

    ax1.set_ylabel('Utilization (%)', fontsize=11, fontweight='bold')
    ax1.set_xlabel('Core ID', fontsize=11, fontweight='bold')
    ax1.set_title('Per-Core Utilization Comparison', fontsize=12, fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels(cores)
    ax1.legend(fontsize=10)
    ax1.grid(axis='y', alpha=0.3)
    ax1.set_ylim(0, 100)

    # Add value labels on bars
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%', ha='center', va='bottom', fontsize=8)

    # Graph 2: Load Balance Index Comparison
    ax2 = plt.subplot(2, 3, 2)
    strategies = ['Dedication', 'PU-WFD']
    lbi_values = [results['Dedication']['lbi'], results['PU-WFD']['lbi']]
    colors = ['#3498db', '#e74c3c']

    bars = ax2.bar(strategies, lbi_values, color=colors, alpha=0.8, width=0.6)
    ax2.set_ylabel('Load Balance Index', fontsize=11, fontweight='bold')
    ax2.set_title('Load Balance Index Comparison', fontsize=12, fontweight='bold')
    ax2.set_ylim(0, 1.0)
    ax2.axhline(y=0.90, color='green', linestyle='--', label='Excellent (>0.90)', alpha=0.5)
    ax2.axhline(y=0.75, color='orange', linestyle='--', label='Good (>0.75)', alpha=0.5)
    ax2.legend(fontsize=9)
    ax2.grid(axis='y', alpha=0.3)

    for bar in bars:
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.3f}', ha='center', va='bottom', fontsize=10, fontweight='bold')

    # Graph 3: Task Distribution per Core
    ax3 = plt.subplot(2, 3, 3)
    x = np.arange(len(cores))

    ded_tasks = results['Dedication']['core_tasks']
    wfd_tasks = results['PU-WFD']['core_tasks']

    bars1 = ax3.bar(x - width/2, ded_tasks, width, label='Dedication', color='#3498db', alpha=0.8)
    bars2 = ax3.bar(x + width/2, wfd_tasks, width, label='PU-WFD', color='#e74c3c', alpha=0.8)

    ax3.set_ylabel('Number of Tasks', fontsize=11, fontweight='bold')
    ax3.set_xlabel('Core ID', fontsize=11, fontweight='bold')
    ax3.set_title('Task Distribution per Core', fontsize=12, fontweight='bold')
    ax3.set_xticks(x)
    ax3.set_xticklabels(cores)
    ax3.legend(fontsize=10)
    ax3.grid(axis='y', alpha=0.3)
    ax3.set_ylim(0, 8)

    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax3.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}', ha='center', va='bottom', fontsize=9, fontweight='bold')

    # Graph 4: Executions per Core
    ax4 = plt.subplot(2, 3, 4)
    x = np.arange(len(cores))

    ded_execs = results['Dedication']['core_executions']
    wfd_execs = results['PU-WFD']['core_executions']

    bars1 = ax4.bar(x - width/2, ded_execs, width, label='Dedication', color='#3498db', alpha=0.8)
    bars2 = ax4.bar(x + width/2, wfd_execs, width, label='PU-WFD', color='#e74c3c', alpha=0.8)

    ax4.set_ylabel('Task Executions (5 sec)', fontsize=11, fontweight='bold')
    ax4.set_xlabel('Core ID', fontsize=11, fontweight='bold')
    ax4.set_title('Task Executions per Core', fontsize=12, fontweight='bold')
    ax4.set_xticks(x)
    ax4.set_xticklabels(cores)
    ax4.legend(fontsize=10)
    ax4.grid(axis='y', alpha=0.3)

    # Graph 5: Utilization Range Comparison
    ax5 = plt.subplot(2, 3, 5)

    ded_range = (max(results['Dedication']['core_utilizations']) -
                 min(results['Dedication']['core_utilizations'])) * 100
    wfd_range = (max(results['PU-WFD']['core_utilizations']) -
                 min(results['PU-WFD']['core_utilizations'])) * 100

    ranges = [ded_range, wfd_range]
    bars = ax5.bar(strategies, ranges, color=colors, alpha=0.8, width=0.6)
    ax5.set_ylabel('Utilization Range (%)', fontsize=11, fontweight='bold')
    ax5.set_title('Load Distribution Evenness\n(Lower is Better)', fontsize=12, fontweight='bold')
    ax5.grid(axis='y', alpha=0.3)

    for bar in bars:
        height = bar.get_height()
        ax5.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.1f}%', ha='center', va='bottom', fontsize=10, fontweight='bold')

    # Graph 6: Safety Metrics (Deadline Misses)
    ax6 = plt.subplot(2, 3, 6)
    quadrants = ['Q1\n(Critical)', 'Q2\n(High Crit)', 'Q3\n(High Urg)', 'Q4\n(Background)']

    ded_misses = [results['Dedication']['q1_misses'], results['Dedication']['q2_misses'],
                  results['Dedication']['q3_misses'], results['Dedication']['q4_misses']]
    wfd_misses = [results['PU-WFD']['q1_misses'], results['PU-WFD']['q2_misses'],
                  results['PU-WFD']['q3_misses'], results['PU-WFD']['q4_misses']]

    x = np.arange(len(quadrants))
    bars1 = ax6.bar(x - width/2, ded_misses, width, label='Dedication', color='#27ae60', alpha=0.8)
    bars2 = ax6.bar(x + width/2, wfd_misses, width, label='PU-WFD', color='#27ae60', alpha=0.8)

    ax6.set_ylabel('Deadline Misses', fontsize=11, fontweight='bold')
    ax6.set_xlabel('Quadrant', fontsize=11, fontweight='bold')
    ax6.set_title('Deadline Misses by Quadrant\n✓ Perfect Safety (All 0)',
                  fontsize=12, fontweight='bold', color='green')
    ax6.set_xticks(x)
    ax6.set_xticklabels(quadrants, fontsize=9)
    ax6.legend(fontsize=10)
    ax6.grid(axis='y', alpha=0.3)
    ax6.set_ylim(0, 5)

    # Add "✓ 0%" labels
    for i, (d, w) in enumerate(zip(ded_misses, wfd_misses)):
        ax6.text(i, 0.5, '✓ 0%', ha='center', va='center', fontsize=10,
                fontweight='bold', color='green')

    plt.suptitle('Multi-Core SMP Performance Analysis\nQuadrant-Based Dedication vs PU-WFD',
                 fontsize=14, fontweight='bold', y=0.995)

    plt.tight_layout(rect=[0, 0.03, 1, 0.97])
    plt.savefig('SMP_RESULTS_GRAPHS.png', dpi=300, bbox_inches='tight')
    print("✓ Created SMP_RESULTS_GRAPHS.png")

    # Create additional detailed visualization
    create_allocation_visualization()

def create_allocation_visualization():
    """Create task allocation visualization"""

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 8))

    # Colors for quadrants
    quad_colors = {
        'Q1': '#e74c3c',  # Red - Critical
        'Q2': '#f39c12',  # Orange
        'Q3': '#3498db',  # Blue
        'Q4': '#95a5a6'   # Gray
    }

    task_quads = {
        'CAN_Handler': 'Q1', 'Brake_Monitor': 'Q1', 'Sensor_Fusion': 'Q1', 'Emergency_Shutdown': 'Q1',
        'Diagnostics': 'Q2', 'Watchdog_Refresh': 'Q2', 'NVM_Logging': 'Q2', 'Health_Monitor': 'Q2',
        'HMI_Render': 'Q3', 'Touch_Handler': 'Q3', 'Audio_Buffer': 'Q3', 'Nav_Update': 'Q3',
        'Telemetry': 'Q4', 'Data_Logging': 'Q4', 'File_Cleanup': 'Q4', 'Statistics': 'Q4'
    }

    def draw_allocation(ax, strategy_name, allocation):
        ax.set_xlim(0, 4)
        ax.set_ylim(0, 8)
        ax.set_xlabel('Core ID', fontsize=12, fontweight='bold')
        ax.set_ylabel('Tasks', fontsize=12, fontweight='bold')
        ax.set_title(f'{strategy_name} Task Allocation', fontsize=13, fontweight='bold')
        ax.set_xticks([0.5, 1.5, 2.5, 3.5])
        ax.set_xticklabels(['Core 0', 'Core 1', 'Core 2', 'Core 3'])

        for core_id in range(4):
            core_key = f'Core {core_id}'
            tasks = allocation[core_key]

            for i, task in enumerate(tasks):
                quad = task_quads.get(task, 'Q4')
                color = quad_colors[quad]

                # Draw task box
                rect = mpatches.Rectangle((core_id, i), 1, 0.9,
                                         facecolor=color, edgecolor='black',
                                         linewidth=1.5, alpha=0.7)
                ax.add_patch(rect)

                # Add task name
                task_short = task.replace('_', '\n')
                ax.text(core_id + 0.5, i + 0.45, task_short,
                       ha='center', va='center', fontsize=7, fontweight='bold')

        ax.grid(False)
        ax.set_aspect('equal')

    draw_allocation(ax1, 'Dedication', results['Dedication']['task_allocation'])
    draw_allocation(ax2, 'PU-WFD', results['PU-WFD']['task_allocation'])

    # Add legend
    legend_elements = [mpatches.Patch(facecolor=quad_colors['Q1'], edgecolor='black', label='Q1 (High Crit + High Urg)'),
                      mpatches.Patch(facecolor=quad_colors['Q2'], edgecolor='black', label='Q2 (High Crit + Low Urg)'),
                      mpatches.Patch(facecolor=quad_colors['Q3'], edgecolor='black', label='Q3 (Low Crit + High Urg)'),
                      mpatches.Patch(facecolor=quad_colors['Q4'], edgecolor='black', label='Q4 (Low Crit + Low Urg)')]

    fig.legend(handles=legend_elements, loc='lower center', ncol=4,
              fontsize=10, frameon=True, bbox_to_anchor=(0.5, -0.05))

    plt.suptitle('Task-to-Core Allocation Visualization', fontsize=14, fontweight='bold')
    plt.tight_layout(rect=[0, 0.05, 1, 0.97])
    plt.savefig('SMP_TASK_ALLOCATION.png', dpi=300, bbox_inches='tight')
    print("✓ Created SMP_TASK_ALLOCATION.png")

if __name__ == '__main__':
    print("\n" + "="*70)
    print("Multi-Core SMP Results Analysis")
    print("="*70 + "\n")

    create_comparison_tables()
    create_visualizations()

    print("\n" + "="*70)
    print("Analysis Complete!")
    print("="*70)
    print("\nGenerated files:")
    print("  1. SMP_RESULTS_TABLES.md - Detailed comparison tables")
    print("  2. SMP_RESULTS_GRAPHS.png - Performance graphs")
    print("  3. SMP_TASK_ALLOCATION.png - Task allocation visualization")
    print("\n")
