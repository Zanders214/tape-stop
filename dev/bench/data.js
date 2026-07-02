window.BENCHMARK_DATA = {
  "lastUpdate": 1782998831409,
  "repoUrl": "https://github.com/Zanders214/tape-stop",
  "entries": {
    "Tape Stop DSP": [
      {
        "commit": {
          "author": {
            "email": "152227414+Zanders214@users.noreply.github.com",
            "name": "Dennis Zanders",
            "username": "Zanders214"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6c689129a8851f7b91604a6023de72702882ffbc",
          "message": "Merge pull request #9 from Zanders214/feat/perf-dashboard\n\nPerformance benchmark + trend dashboard (nanobench)",
          "timestamp": "2026-06-25T19:52:30+03:00",
          "tree_id": "71b6089eab7369113a3e53f8837a251de1980fcd",
          "url": "https://github.com/Zanders214/tape-stop/commit/6c689129a8851f7b91604a6023de72702882ffbc"
        },
        "date": 1782406548672,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "processBlock",
            "value": 18652.319,
            "unit": "ns/block"
          },
          {
            "name": "DSP load @48k/512",
            "value": 0.175,
            "unit": "%"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "152227414+Zanders214@users.noreply.github.com",
            "name": "Dennis Zanders",
            "username": "Zanders214"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "87b68cb6ff874a1c4458f963dff6412aae96750e",
          "message": "Merge pull request #10 from Zanders214/feat/dsp-pow-cache\n\nSkip per-sample std::pow in the DSP steady state",
          "timestamp": "2026-06-25T20:55:48+03:00",
          "tree_id": "1e321571df990aed325cee2ece80ddc12db8cb2f",
          "url": "https://github.com/Zanders214/tape-stop/commit/87b68cb6ff874a1c4458f963dff6412aae96750e"
        },
        "date": 1782410447672,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "processBlock",
            "value": 4040.08,
            "unit": "ns/block"
          },
          {
            "name": "DSP load @48k/512",
            "value": 0.038,
            "unit": "%"
          },
          {
            "name": "processBlock (running)",
            "value": 4851.244,
            "unit": "ns/block"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "152227414+Zanders214@users.noreply.github.com",
            "name": "Dennis Zanders",
            "username": "Zanders214"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": false,
          "id": "87b68cb6ff874a1c4458f963dff6412aae96750e",
          "message": "Merge pull request #10 from Zanders214/feat/dsp-pow-cache\n\nSkip per-sample std::pow in the DSP steady state",
          "timestamp": "2026-06-25T20:55:48+03:00",
          "tree_id": "1e321571df990aed325cee2ece80ddc12db8cb2f",
          "url": "https://github.com/Zanders214/tape-stop/commit/87b68cb6ff874a1c4458f963dff6412aae96750e"
        },
        "date": 1782997252059,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "processBlock",
            "value": 4073.123,
            "unit": "ns/block"
          },
          {
            "name": "DSP load @48k/512",
            "value": 0.038,
            "unit": "%"
          },
          {
            "name": "processBlock (running)",
            "value": 4884.408,
            "unit": "ns/block"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "152227414+Zanders214@users.noreply.github.com",
            "name": "Dennis Zanders",
            "username": "Zanders214"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c0fe0ecc8eb14374b20bcc08e8f25207af5f857b",
          "message": "Merge pull request #11 from Zanders214/feat/macos-installer\n\nAdd macOS .pkg installer build script",
          "timestamp": "2026-07-02T15:24:02+02:00",
          "tree_id": "593ee778790de5894e277adc15568719c06e1d1b",
          "url": "https://github.com/Zanders214/tape-stop/commit/c0fe0ecc8eb14374b20bcc08e8f25207af5f857b"
        },
        "date": 1782998830496,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "processBlock",
            "value": 4398.478,
            "unit": "ns/block"
          },
          {
            "name": "DSP load @48k/512",
            "value": 0.041,
            "unit": "%"
          },
          {
            "name": "processBlock (running)",
            "value": 5007.679,
            "unit": "ns/block"
          }
        ]
      }
    ]
  }
}