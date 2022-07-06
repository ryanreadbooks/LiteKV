import csv
import os
import os
import matplotlib as mpl
mpl.use('TkAgg')
import matplotlib.pyplot as plt
import glob
import numpy as np

if __name__ == '__main__':
    # folder = 'single-thread'
    # folder = 'multi-threads'
    for folder in ('single-thread', 'multi-threads'):
        if not os.path.exists(folder):
            continue
        subfolders = glob.glob(os.path.join(folder, '*'))
        print('subfolders = ', subfolders)
        headers = []
        client_stat = {}
        for subfolder in subfolders:
            csv_filenames = sorted(glob.glob(os.path.join(subfolder, '*.csv')))
            print('csv_filenames = ', csv_filenames)
            ans = {}
            for csv_filename in csv_filenames:
                # every csv file
                with open(csv_filename, 'r', newline='') as file:
                    # first line is header
                    headers = file.readline()
                    headers = headers.strip().split(',')
                    headers = [i[1:-1] for i in headers][1:]
                    datas = file.readlines()
                    for data in datas:
                        dl = data.strip().split(',')
                        cmd = dl[0][1:-1]
                        digits = [float(i[1:-1].strip()) for i in dl[1:]]
                        digits = np.array(digits)
                        if not ans.__contains__(cmd):
                            ans[cmd] = digits
                        else:
                            ans[cmd] += digits
            # print(ans)
            for k, v in ans.items():
                ans[k] = ans[k] / len(csv_filenames)
            client_stat[os.path.basename(subfolder)[2:]] = ans
        # print(client_stat)

        total = {}
        for k, v in client_stat.items():
            # variant: number of clients
            c = float(k)
            for kk, vv in client_stat[k].items():
                if not total.__contains__(kk):
                    total[kk] = {}
                for value, header in zip(vv, headers):
                    if not total[kk].__contains__(header):
                        total[kk][header] = {}
                    total[kk][header][c] = value

        # print(total)
        metrics = {}
        for command, v in total.items():
            for header, value_dict in v.items():
                for c, div in value_dict.items():
                    # print(command, ' => ', header, ' -> ', c, ' -- ', div)
                    if not metrics.__contains__(header):
                        metrics[header] = {}
                    if not metrics[header].__contains__(command):
                        metrics[header][command] = {}
                    metrics[header][command][c] = div

        # plot figure
        marker_candidates = ['.', ',', 'o', 'v', '^', '<', '>', 's', 'p', '*', '+', 'x', 'D', 'd', '_', 'P', 'X']
        linestyle_tuple = [
            'solid',
            (0, (1, 10)),
            (0, (1, 1)),
            (0, (1, 1)),

            (0, (5, 10)),
            (0, (5, 5)),
            (0, (5, 1)),

            (0, (3, 10, 1, 10)),
            (0, (3, 5, 1, 5)),
            (0, (3, 1, 1, 1)),

            (0, (3, 5, 1, 5, 1, 5)),
            (0, (3, 10, 1, 10, 1, 10)),
            (0, (3, 1, 1, 1, 1, 1))]

        for i, header in enumerate(headers):
            plt.figure(num=i + 1)
            # ax = plt.subplot(3, 3, i + 1, title=header)
            metric = metrics[header]
            for j, (cmd, datas) in enumerate(metric.items()):
                x_labels = []
                y_labels = []
                for x_value, y_value in datas.items():
                    x_labels.append(x_value)
                    y_labels.append(y_value)
                print(x_labels, ' ', y_labels)
                # sort x_labels
                x_labels = np.array(x_labels)
                y_labels = np.array(y_labels)
                sort_idx = np.argsort(x_labels)
                x_labels = x_labels[sort_idx]
                y_labels = y_labels[sort_idx]
                plt.plot(x_labels, y_labels, label=cmd, marker=marker_candidates[j], ls=linestyle_tuple[j])
            plt.legend()
            if folder == 'single-thread':
                plt.xlabel('Number of clients')
                extra = ' (n_threads=1, n_requests=100000)'
            else:
                plt.xlabel('Number of threads')
                extra = ' (n_clients=1, n_requests=100000)'
            plt.title(header + extra)
            if header == 'rps':
                plt.ylabel('Requests per second')
            else:
                plt.ylabel('Latency(ms)')

        plt.show()
