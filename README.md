# Adaptive-Parallel

I love the [GNU Parallel](https://www.gnu.org/software/parallel/) program but it requires two things I don't like to do:

1. Think about how many jobs I want to run in parallel and specify it with the -j option
2. Think about all the jobs I want to start. I can't add jobs later, I either have to stop the running jobs and re-start parallel with all the jobs or start a second instance of parallel but then I'd potentially have too many processes competing for resources like RAM and disk access.

Therefore I wrote this program. Roughly every 10 seconds it checks the current 1-minute [load](https://en.wikipedia.org/wiki/Load_(computing)) average and the [CPU usage](https://github.com/vivaladav/BitsOfBytes/tree/master/cpp-program-to-get-cpu-usage-from-command-line-in-linux). If both are below their respective thresholds the next job is started followed by a 40 second wait.

## Usage

Example, converting a bunch of tif files to jpg using imagemagick:
```
for i in *tif; do echo "convert $i $i.jpg"; done | adaptive-parallel
```

This only makes sense if each job takes much longer than a minute to complete.
On the other hand it has the advantage that you can run multiple instances in case you forgot some jobs when you started the first instance. The order of completion will be random but since all instances check the system load before starting a process the total load should remain manageable.

